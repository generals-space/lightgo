package parser_test

import (
	"fmt"
	"go/ast"
	"go/parser"
	"go/token"
	"strings"
)

var sourceCode1 string = `
package main

import "fmt"

func main() {
    fmt.Println("Hello, world!")
}
`

func ExampleParseFileInspect() {
	// 解析源文件
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "", sourceCode1, 0)
	if err != nil {
		panic(err)
	}

	// 遍历AST
	// ast.Inspect(f, func(node ast.Node) bool {
	// 	if node == nil {
	// 		return true
	// 	}
	// 	// 处理节点
	// 	fmt.Println(node)
	// 	return true
	// })

	ast.Print(fset, f)

	// Output:
}

var sourceCode2 string = `
// package main
package main /* the name is main */
import "fmt"

type StcB struct {
	A, B, C int // associated with a, b, c
	// associated with x, y
	x, y float64    // float values
	z    complex128 // complex value
}
`

func ExampleParseFileInspect2() {
	// 解析源文件
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "", sourceCode2, 0)
	if err != nil {
		panic(err)
	}

	for _, v := range f.Decls {
		var (
			structComment string
			structName    string
			fieldNumber   int
		)
		if stc, ok := v.(*ast.GenDecl); ok && stc.Tok == token.TYPE {
			//fmt.Println(stc.Tok) //import、type、struct...
			if stc.Doc != nil {
				structComment = strings.TrimRight(stc.Doc.Text(), "\n")
			}
			for _, spec := range stc.Specs {
				if tp, ok := spec.(*ast.TypeSpec); ok {
					structName = tp.Name.Name
					fmt.Println("结构体名称:", structName)
					fmt.Println("结构体注释:", structComment)
					if stp, ok := tp.Type.(*ast.StructType); ok {
						if !stp.Struct.IsValid() {
							continue
						}
						fieldNumber = stp.Fields.NumFields()
						fmt.Println("字段数:", fieldNumber)
						for _, field := range stp.Fields.List {
							var (
								fieldName    string
								fieldType    string
								fieldTag     string
								fieldTagKind string
								fieldComment string
							)
							//获取字段名
							if len(field.Names) == 1 {
								fieldName = field.Names[0].Name //等同于field.Names[0].String()) //获取名称方法2
							} else if len(field.Names) > 1 {
								for _, name := range field.Names {
									fieldName = fieldName + name.String() + ","
								}
							}
							if field.Tag != nil {
								fieldTag = field.Tag.Value
								fieldTagKind = field.Tag.Kind.String()
							}
							if field.Comment != nil {
								fieldComment = strings.TrimRight(field.Comment.Text(), "\n")
							}
							if ft, ok := field.Type.(*ast.Ident); ok {
								fieldType = ft.Name
							}
							fmt.Println("\t字段名:", fieldName)
							fmt.Println("\t字段类型:", fieldType)
							fmt.Println("\t标签:", fieldTag, "标签类型", fieldTagKind)
							fmt.Println("\t字段注释:", fieldComment)
							fmt.Println("\t----------")
						}
					}
				}
			}
		}
	}
	// Output:
}
