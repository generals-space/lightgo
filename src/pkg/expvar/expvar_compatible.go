package expvar

import "net/http"

// 	@compatible: 此方法在 v1.8 版本添加
//
// Handler returns the expvar HTTP Handler.
//
// This is only needed to install the handler in a non-standard location.
func Handler() http.Handler {
	return http.HandlerFunc(expvarHandler)
}
