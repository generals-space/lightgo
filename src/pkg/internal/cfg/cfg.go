// 	@compatible: 此文件在 v1.9 版本添加

package cfg

// 	@compatible: 此结构在 v1.9 版本添加
//
// An EnvVar is an environment variable Name=Value.
type EnvVar struct {
	Name  string
	Value string
}
