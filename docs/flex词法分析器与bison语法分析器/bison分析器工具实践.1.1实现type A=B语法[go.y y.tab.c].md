# bison分析器工具实践.1.实现type A=B语法[go.y y.tab.c]


```
(gdb) set args /usr/local/go/gopath/src/test/main.go
(gdb) b src/cmd/gc/typecheck1.c:1539
```

```
src/cmd/gc/declare_declare.c
typedcl1()
|
--  src/cmd/gc/typecheck1.c
    typecheck1(n) -> case ODCLTYPE{}
    |
    | 此时有如下条件成立 [n->left = A, n->right = nil, n->ntype = B, n->op = ODCLTYPE]
    |
	-- typecheck(), 参数为 n->left, 以下称 n
		|
		| n->sym->name = A, n->orig->sym->name = A, n->ntype-sym->name = B, n->op = OTYPE, n->type = nil
		|
		--  src/cmd/gc/typecheck1.c
		    typecheck1(n)
		    |
			| n->sym->name = A, n->orig->sym->name = A, n->ntype->sym->name = B, n->op = OTYPE, n->type = nil
			|
			--  src/cmd/gc/typecheck_def.c
			|   typecheckdef() -> case OTYPE{}
			|   |
			|   | 为 n->type 赋值
			|   |
			|   --  src/cmd/gc/typecheck_deftype.c
			|       typecheckdeftype(n)
			|       |
			|       | n->sym->name = A, n->ntype->sym->name = B, t->sym->name = B
			|       |
			|       --  copytype(n, t)
			|       |
			|       |
			|       |
			|
```

最初 A 和 B 都只是 Node, 是在哪里将ta们声明为 Type 的?

可能可行的方案:

1. 声明 type A = B 时, 将 A->orig 设置为 B, 表示两者底层一致;
2. 声明时拷贝 B 的所有方法, 赋值给 A;
3. 声明时直接将 A 改为 B 的拷贝, 只改一个 sym 名字;
