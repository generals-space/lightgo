package reflect

// 	@compatible: 在 v1.5 版本中, 作了如下改动 arrayOf() -> ArrayOf()
// 	为了改动最少, 这里直接调用原本的 arrayOf(), 有什么问题以后再说.
//
func ArrayOf(count int, elem Type) Type {
	return arrayOf(count, elem)
}
