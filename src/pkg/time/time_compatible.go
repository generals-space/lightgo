package time

// 	@compatible: 此方法在 v1.8 版本初次添加.
//
// Until returns the duration until t.
// It is shorthand for t.Sub(time.Now()).
func Until(t Time) Duration {
	return t.Sub(Now())
}
