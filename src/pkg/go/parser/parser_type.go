package parser

import (
	"go/ast"
	"go/token"
)

// ----------------------------------------------------------------------------
// Types

func (p *parser) parseType() ast.Expr {
	if p.trace {
		defer un(trace(p, "Type"))
	}

	typ := p.tryType()

	if typ == nil {
		pos := p.pos
		p.errorExpected(pos, "type")
		p.next() // make progress
		return &ast.BadExpr{From: pos, To: p.pos}
	}

	return typ
}

// If the result is an identifier, it is not resolved.
func (p *parser) parseTypeName() ast.Expr {
	if p.trace {
		defer un(trace(p, "TypeName"))
	}

	ident := p.parseIdent()
	// don't resolve ident yet - it may be a parameter or field name

	if p.tok == token.PERIOD {
		// ident is a package name
		p.next()
		p.resolve(ident)
		sel := p.parseIdent()
		return &ast.SelectorExpr{X: ident, Sel: sel}
	}

	return ident
}

func (p *parser) parseArrayType() ast.Expr {
	if p.trace {
		defer un(trace(p, "ArrayType"))
	}

	lbrack := p.expect(token.LBRACK)
	var len ast.Expr
	// always permit ellipsis for more fault-tolerant parsing
	if p.tok == token.ELLIPSIS {
		len = &ast.Ellipsis{Ellipsis: p.pos}
		p.next()
	} else if p.tok != token.RBRACK {
		len = p.parseRhs()
	}
	p.expect(token.RBRACK)
	elt := p.parseType()

	return &ast.ArrayType{Lbrack: lbrack, Len: len, Elt: elt}
}

func (p *parser) makeIdentList(list []ast.Expr) []*ast.Ident {
	idents := make([]*ast.Ident, len(list))
	for i, x := range list {
		ident, isIdent := x.(*ast.Ident)
		if !isIdent {
			if _, isBad := x.(*ast.BadExpr); !isBad {
				// only report error if it's a new one
				p.errorExpected(x.Pos(), "identifier")
			}
			ident = &ast.Ident{NamePos: x.Pos(), Name: "_"}
		}
		idents[i] = ident
	}
	return idents
}

func (p *parser) parseFieldDecl(scope *ast.Scope) *ast.Field {
	if p.trace {
		defer un(trace(p, "FieldDecl"))
	}

	doc := p.leadComment

	// FieldDecl
	list, typ := p.parseVarList(false)

	// Tag
	var tag *ast.BasicLit
	if p.tok == token.STRING {
		tag = &ast.BasicLit{ValuePos: p.pos, Kind: p.tok, Value: p.lit}
		p.next()
	}

	// analyze case
	var idents []*ast.Ident
	if typ != nil {
		// IdentifierList Type
		idents = p.makeIdentList(list)
	} else {
		// ["*"] TypeName (AnonymousField)
		typ = list[0] // we always have at least one element
		if n := len(list); n > 1 || !isTypeName(deref(typ)) {
			pos := typ.Pos()
			p.errorExpected(pos, "anonymous field")
			typ = &ast.BadExpr{From: pos, To: list[n-1].End()}
		}
	}

	p.expectSemi() // call before accessing p.linecomment

	field := &ast.Field{Doc: doc, Names: idents, Type: typ, Tag: tag, Comment: p.lineComment}
	p.declare(field, nil, scope, ast.Var, idents...)
	p.resolve(typ)

	return field
}

func (p *parser) parseStructType() *ast.StructType {
	if p.trace {
		defer un(trace(p, "StructType"))
	}

	pos := p.expect(token.STRUCT)
	lbrace := p.expect(token.LBRACE)
	scope := ast.NewScope(nil) // struct scope
	var list []*ast.Field
	for p.tok == token.IDENT || p.tok == token.MUL || p.tok == token.LPAREN {
		// a field declaration cannot start with a '(' but we accept
		// it here for more robust parsing and better error messages
		// (parseFieldDecl will check and complain if necessary)
		list = append(list, p.parseFieldDecl(scope))
	}
	rbrace := p.expect(token.RBRACE)

	return &ast.StructType{
		Struct: pos,
		Fields: &ast.FieldList{
			Opening: lbrace,
			List:    list,
			Closing: rbrace,
		},
	}
}

func (p *parser) parsePointerType() *ast.StarExpr {
	if p.trace {
		defer un(trace(p, "PointerType"))
	}

	star := p.expect(token.MUL)
	base := p.parseType()

	return &ast.StarExpr{Star: star, X: base}
}

// If the result is an identifier, it is not resolved.
func (p *parser) tryVarType(isParam bool) ast.Expr {
	if isParam && p.tok == token.ELLIPSIS {
		pos := p.pos
		p.next()
		typ := p.tryIdentOrType() // don't use parseType so we can provide better error message
		if typ != nil {
			p.resolve(typ)
		} else {
			p.error(pos, "'...' parameter is missing type")
			typ = &ast.BadExpr{From: pos, To: p.pos}
		}
		return &ast.Ellipsis{Ellipsis: pos, Elt: typ}
	}
	return p.tryIdentOrType()
}

// If the result is an identifier, it is not resolved.
func (p *parser) parseVarType(isParam bool) ast.Expr {
	typ := p.tryVarType(isParam)
	if typ == nil {
		pos := p.pos
		p.errorExpected(pos, "type")
		p.next() // make progress
		typ = &ast.BadExpr{From: pos, To: p.pos}
	}
	return typ
}

// If any of the results are identifiers, they are not resolved.
func (p *parser) parseVarList(isParam bool) (list []ast.Expr, typ ast.Expr) {
	if p.trace {
		defer un(trace(p, "VarList"))
	}

	// a list of identifiers looks like a list of type names
	//
	// parse/tryVarType accepts any type (including parenthesized
	// ones) even though the syntax does not permit them here: we
	// accept them all for more robust parsing and complain later
	for typ := p.parseVarType(isParam); typ != nil; {
		list = append(list, typ)
		if p.tok != token.COMMA {
			break
		}
		p.next()
		typ = p.tryVarType(isParam) // maybe nil as in: func f(int,) {}
	}

	// if we had a list of identifiers, it must be followed by a type
	typ = p.tryVarType(isParam)

	return
}

func (p *parser) parseParameterList(scope *ast.Scope, ellipsisOk bool) (params []*ast.Field) {
	if p.trace {
		defer un(trace(p, "ParameterList"))
	}

	// ParameterDecl
	list, typ := p.parseVarList(ellipsisOk)

	// analyze case
	if typ != nil {
		// IdentifierList Type
		idents := p.makeIdentList(list)
		field := &ast.Field{Names: idents, Type: typ}
		params = append(params, field)
		// Go spec: The scope of an identifier denoting a function
		// parameter or result variable is the function body.
		p.declare(field, nil, scope, ast.Var, idents...)
		p.resolve(typ)
		if p.tok == token.COMMA {
			p.next()
		}
		for p.tok != token.RPAREN && p.tok != token.EOF {
			idents := p.parseIdentList()
			typ := p.parseVarType(ellipsisOk)
			field := &ast.Field{Names: idents, Type: typ}
			params = append(params, field)
			// Go spec: The scope of an identifier denoting a function
			// parameter or result variable is the function body.
			p.declare(field, nil, scope, ast.Var, idents...)
			p.resolve(typ)
			if !p.atComma("parameter list") {
				break
			}
			p.next()
		}
	} else {
		// Type { "," Type } (anonymous parameters)
		params = make([]*ast.Field, len(list))
		for i, typ := range list {
			p.resolve(typ)
			params[i] = &ast.Field{Type: typ}
		}
	}

	return
}

func (p *parser) parseParameters(scope *ast.Scope, ellipsisOk bool) *ast.FieldList {
	if p.trace {
		defer un(trace(p, "Parameters"))
	}

	var params []*ast.Field
	lparen := p.expect(token.LPAREN)
	if p.tok != token.RPAREN {
		params = p.parseParameterList(scope, ellipsisOk)
	}
	rparen := p.expect(token.RPAREN)

	return &ast.FieldList{Opening: lparen, List: params, Closing: rparen}
}

func (p *parser) parseResult(scope *ast.Scope) *ast.FieldList {
	if p.trace {
		defer un(trace(p, "Result"))
	}

	if p.tok == token.LPAREN {
		return p.parseParameters(scope, false)
	}

	typ := p.tryType()
	if typ != nil {
		list := make([]*ast.Field, 1)
		list[0] = &ast.Field{Type: typ}
		return &ast.FieldList{List: list}
	}

	return nil
}

func (p *parser) parseSignature(scope *ast.Scope) (params, results *ast.FieldList) {
	if p.trace {
		defer un(trace(p, "Signature"))
	}

	params = p.parseParameters(scope, true)
	results = p.parseResult(scope)

	return
}

func (p *parser) parseFuncType() (*ast.FuncType, *ast.Scope) {
	if p.trace {
		defer un(trace(p, "FuncType"))
	}

	pos := p.expect(token.FUNC)
	scope := ast.NewScope(p.topScope) // function scope
	params, results := p.parseSignature(scope)

	return &ast.FuncType{Func: pos, Params: params, Results: results}, scope
}

func (p *parser) parseMethodSpec(scope *ast.Scope) *ast.Field {
	if p.trace {
		defer un(trace(p, "MethodSpec"))
	}

	doc := p.leadComment
	var idents []*ast.Ident
	var typ ast.Expr
	x := p.parseTypeName()
	if ident, isIdent := x.(*ast.Ident); isIdent && p.tok == token.LPAREN {
		// method
		idents = []*ast.Ident{ident}
		scope := ast.NewScope(nil) // method scope
		params, results := p.parseSignature(scope)
		typ = &ast.FuncType{Func: token.NoPos, Params: params, Results: results}
	} else {
		// embedded interface
		typ = x
		p.resolve(typ)
	}
	p.expectSemi() // call before accessing p.linecomment

	spec := &ast.Field{Doc: doc, Names: idents, Type: typ, Comment: p.lineComment}
	p.declare(spec, nil, scope, ast.Fun, idents...)

	return spec
}

func (p *parser) parseInterfaceType() *ast.InterfaceType {
	if p.trace {
		defer un(trace(p, "InterfaceType"))
	}

	pos := p.expect(token.INTERFACE)
	lbrace := p.expect(token.LBRACE)
	scope := ast.NewScope(nil) // interface scope
	var list []*ast.Field
	for p.tok == token.IDENT {
		list = append(list, p.parseMethodSpec(scope))
	}
	rbrace := p.expect(token.RBRACE)

	return &ast.InterfaceType{
		Interface: pos,
		Methods: &ast.FieldList{
			Opening: lbrace,
			List:    list,
			Closing: rbrace,
		},
	}
}

func (p *parser) parseMapType() *ast.MapType {
	if p.trace {
		defer un(trace(p, "MapType"))
	}

	pos := p.expect(token.MAP)
	p.expect(token.LBRACK)
	key := p.parseType()
	p.expect(token.RBRACK)
	value := p.parseType()

	return &ast.MapType{Map: pos, Key: key, Value: value}
}

func (p *parser) parseChanType() *ast.ChanType {
	if p.trace {
		defer un(trace(p, "ChanType"))
	}

	pos := p.pos
	dir := ast.SEND | ast.RECV
	var arrow token.Pos
	if p.tok == token.CHAN {
		p.next()
		if p.tok == token.ARROW {
			arrow = p.pos
			p.next()
			dir = ast.SEND
		}
	} else {
		arrow = p.expect(token.ARROW)
		p.expect(token.CHAN)
		dir = ast.RECV
	}
	value := p.parseType()

	return &ast.ChanType{Begin: pos, Arrow: arrow, Dir: dir, Value: value}
}

// If the result is an identifier, it is not resolved.
func (p *parser) tryIdentOrType() ast.Expr {
	switch p.tok {
	case token.IDENT:
		return p.parseTypeName()
	case token.LBRACK:
		return p.parseArrayType()
	case token.STRUCT:
		return p.parseStructType()
	case token.MUL:
		return p.parsePointerType()
	case token.FUNC:
		typ, _ := p.parseFuncType()
		return typ
	case token.INTERFACE:
		return p.parseInterfaceType()
	case token.MAP:
		return p.parseMapType()
	case token.CHAN, token.ARROW:
		return p.parseChanType()
	case token.LPAREN:
		lparen := p.pos
		p.next()
		typ := p.parseType()
		rparen := p.expect(token.RPAREN)
		return &ast.ParenExpr{Lparen: lparen, X: typ, Rparen: rparen}
	}

	// no type found
	return nil
}

func (p *parser) tryType() ast.Expr {
	typ := p.tryIdentOrType()
	if typ != nil {
		p.resolve(typ)
	}
	return typ
}
