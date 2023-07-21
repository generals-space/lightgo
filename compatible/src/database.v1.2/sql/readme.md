//  @compatible: 由于 database 库不属于核心库, 因此将 database 整个目录移出 src 目录, 放到 compatible 目录中.
// 又因为 database 库在 v1.8, v1.9 版本发生较大变动, 导致向后兼容过于困难, 因此将原 database 目录放到 compatible/src/database.v1.2 中, 
// 而引入 v1.9 版本的 database 放到 compatible/src/database 进行替换.
