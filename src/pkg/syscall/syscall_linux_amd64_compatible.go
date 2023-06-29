package syscall

// 关于 Gettimeofday() 与 gettimeofday() 的变更关系如下:
//
// v1.2: Gettimeofday() -------------------> ·Gettimeofday()
// 
// v1.3: Gettimeofday() -> gettimeofday() -> ·gettimeofday()
//

// 	@compatible: 该方法在 v1.3 版本初次添加, 其实与 Gettimeofday() 作用相同
// 	@implementAt: src/pkg/syscall/asm_linux_amd64_compatible.s -> ·gettimeofday()
//
//go:noescape
func gettimeofday(tv *Timeval) (err Errno)

// 	@compatible: 该方法在 v1.3 版本中被改为如下.
//
// func Gettimeofday(tv *Timeval) (err error) {
// 	errno := gettimeofday(tv)
// 	if errno != 0 {
// 		return errno
// 	}
// 	return nil
// }
