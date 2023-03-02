package main

type P interface{}

type Person interface {
	Greet()
}

type Student struct {
	Name string
}

func(s *Student) Greet() {
	println(s.Name)
}

type BadStudent struct {
	Name string
}

func func01(){
	// 此处隐含了 *struct -> interface{} 的类型转换过程
	// 实际调用了 src/pkg/runtime/iface.c -> runtime·typ2Itab()
	var student Person = &Student{Name: "general"}
	student.Greet()
}

// func func02(){
// 	/*
// 		// 这种编译层面错误无法通过 recover() 捕获.
// 		defer func() {
// 			if err := recover(); err != nil {
// 				println(err)
// 			}
// 		}()
// 	*/
// 	var student Person = &BadStudent{Name: "general"}
// 	student.Greet()
// }

func func03(){
	// 如下2行可能会和下下面2行连起来, 无法在 debug 时通过 n 指令一条一条调试, 
	// 可以使用"b iface.c:行号"实现分隔.
	var istudent Person = &Student{Name: "general"}
	istudent.Greet()
	// 这一行, 调用了 src/pkg/runtime/iface.c -> runtime·assertI2T2()
	student, _ := istudent.(*Student)
	student.Greet()
	// 本来以为下面加一个"ok"会调用到 src/pkg/runtime/iface.c -> runtime·assertI2TOK()
	// 结果不行
	student2, ok := istudent.(*Student)
	println(ok)
	student2.Greet()

}

func func04(){
	var i int
	var pi P = i
	println(pi)
	// 这一行, 调用了 src/pkg/runtime/iface.c -> runtime·assertE2T2()
	// 对于没有方法的接口断言, 对应的是 Eface 类型.
	ii, _ := pi.(int)
	println(ii)
}

func main() {
	// func01()
	// func02()
	// func03()
	func04()
}
