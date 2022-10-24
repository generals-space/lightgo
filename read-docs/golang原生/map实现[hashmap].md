# map实现

参考文章

1. [Golang map 的底层实现](https://www.jianshu.com/p/aa0d4808cbb8)
    - map 的底层的存储结构图
2. [哈希表的原理](https://www.cnblogs.com/funtrin/p/16060350.html)


在golang中, map的底层实现名为hashmap, 本质上由数组+链表完成.

```
    Hmap
+------------+
|   count    |
+------------+
|   hash0    |
+------------+
|     B      |
+------------+
|  keysize   |
+------------+
| valuesize  |          Bucket     Bucket
+------------+--------+---------+---------+---------+ .... +---------+
| bucketsize |        | tophash | tophash | tophash | .... | tophash |
+------------+        +---------+---------+---------+ .... +---------+----------+
|  buckets   |        |  data   |  data   |  data   | .... |  data   |          |
+------------+        +---------+---------+---------+ .... +---------+          |
                                                                                |
                      |<--------- bucketsize 个 Bucket 成员 --------->|          |
                                                                                |
                                                                                |
                                                            +-------+-----------+
                                                            |  key  |     连续
                                                            |  key  |  BUCKETSIZE
                                                            |  ...  |   个 key
                                                            |  key  |
                                                            +-------+
                                                            |  val  |    连续
                                                            |  val  |  BUCKETSIZE
                                                            |  ...  |   个 value
                                                            |  val  |
                                                            +-------+
                                                        这种连续的 key 与 value, 而不是 key value key value...
                                                        是为了内存对齐
```
