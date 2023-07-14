package x509

// 	@compatible: 此函数在 v1.7 版本添加.
//
// SystemCertPool returns a copy of the system cert pool.
//
// Any mutations to the returned pool are not written to disk and do
// not affect any other pool.
func SystemCertPool() (*CertPool, error) {
	// 	@note: 为保证改动最小, 函数体有变动.
	//
	// if runtime.GOOS == "windows" {
	// 	return nil, errors.New("crypto/x509: system root pool is not available on Windows")
	// }
	// return loadSystemRoots()

	return systemRootsPool(), nil
}
