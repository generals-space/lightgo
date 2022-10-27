类型声明应该是在 src/pkg/runtime/type.h 中, 包含 MapType, SliceType, FuncType 等.

slice 的初始化函数: src/pkg/runtime/slice.c -> runtime·makeslice()
map 的初始化函数: src/pkg/runtime/hashmap.c -> runtime·makemap()
channel 的初始化函数: src/pkg/runtime/chan.c -> runtime·makechan()
go func 初始化函数: src/pkg/runtime/proc.c -> runtime·newproc()


还有 src/pkg/runtime/runtime.h, 包含 GMP, String, Slice, Func, Defer 等对象.

------

还有 src/pkg/reflect/type.go -> arrayType{}, funcType{}, interfaceType{}, mapType{}, sliceType{}, structType{}, ptrType{}

感觉这这里的类型定义, 是引用类型? 

将 runtime 层面的变量信息, 封装后给 golang 层面看的, 其中也包含了底层的信息(比如 hmap 的 bucket 信息).
