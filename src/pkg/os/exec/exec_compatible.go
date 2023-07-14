package exec

// 	@compatible: 该函数在 v1.7 版本初次添加.
//
// 	@param ctx: 暂时没用到, 直接丢弃了.
//
// CommandContext is like Command but includes a context.
//
// The provided context is used to kill the process (by calling
// os.Process.Kill) if the context becomes done before the command
// completes on its own.
// func CommandContext(ctx context.Context, name string, arg ...string) *Cmd {
func CommandContext(ctx interface{}, name string, arg ...string) *Cmd {
	if ctx == nil {
		panic("nil Context")
	}
	cmd := Command(name, arg...)
	// 暂不添加该字段
	// cmd.ctx = ctx
	return cmd
}
