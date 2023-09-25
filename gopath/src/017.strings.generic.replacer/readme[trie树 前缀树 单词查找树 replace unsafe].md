`replace_generic.go`拷贝自`src/pkg/strings/replace_generic.go`.

由于在调试过程需要打印生成的前缀树, 但 encoding/json 引用了 strings 库, 因此后者无法再直接引用前者了(会引发循环引用的问题), 这里将整个文件直接拷贝过来,

为了使用`json.Marshal()`打印整个前缀树结构, 还需要将`strings.trieNode{}`结构体中所有成员字段设置为首字母大写, 否则只能打印为结果`{}`.

## 样例分析

- flag => pkg/flag
- fmt  => pkg/fmt
- go   => pkg/go

### 前缀树分析

创建完`Replacer`后的前缀树结构如下,

```json
{
    "Value": "",
    "Priority": 0,
    "Prefix": "",
    "Next": null,
    "Table": [
        null,
        {
            "Value": "",
            "Priority": 0,
            "Prefix": "",
            "Next": null,
            "Table": [
                null,
                null,
                null,
                {
                    "Value": "",
                    "Priority": 0,
                    "Prefix": "ag",
                    "Next": {
                        "Value": "pkg/flag",
                        "Priority": 6,
                        "Prefix": "",
                        "Next": null,
                        "Table": null
                    },
                    "Table": null
                },
                {
                    "Value": "",
                    "Priority": 0,
                    "Prefix": "t",
                    "Next": {
                        "Value": "pkg/fmt",
                        "Priority": 4,
                        "Prefix": "",
                        "Next": null,
                        "Table": null
                    },
                    "Table": null
                },
                null,
                null
            ]
        },
        {
            "Value": "",
            "Priority": 0,
            "Prefix": "o",
            "Next": {
                "Value": "pkg/go",
                "Priority": 2,
                "Prefix": "",
                "Next": null,
                "Table": null
            },
            "Table": null
        },
        null,
        null,
        null,
        null
    ]
}
```

其中每个层级的`Table`节点, 都拥有`tableSize`个成员(示例里是`7`). 

每个成员都对应`old`中的一个字母, 比如样例中的old字符串有"flag", "fmt", "go", 一共有7个字符(对应`tableSize`), 按照ASCII码表中的顺序分别是: `a: 0, f: 1, g: 2, l: 3, m: 4, o: 5, t: 6`

```
level0(root本身)

level1                          [a: 0, f: 1, g: 2, l: 3, m: 4, o: 5, t: 6]
                                [null, f: 1, g: 2, null, null, null, null]
                                       |     |
                               ┌───────┤     └─────────────────────────────────────────┐
                               |     ┌─┘                                               |
level2      [a: 0, f: 1, g: 2, l: 3, m: 4, o: 5, t: 6], [a: 0, f: 1, g: 2, l: 3, m: 4, o: 5, t: 6]
            [null, null, null, l: 3, m: 4, null, null], [null, null, null, null, null, o: 5, null]
                               |     |                                                 |
                               |     |                                                 |
                               |     |      {"Prefix": "o", "Next": {"Value": "pkg/go"}}
                               |     |
                               |     {"Prefix": "t", "Next": {"Value": "pkg/fmt"}}
                               |
                               {"Prefix": "ag", "Next": {"Value": "pkg/flag"}}
```

拥有`Value`的可以看作是叶子节点.

简化一下

```
               root
            /        \
         f              g
     /       \          |
    l         m         |
    |         |         |
    ag        t         o
    |         |         |
 pkg/flag  pkg/fmt    pkg/go
```

这样, 所有old字符串都存储在了`Trie`前缀树中.

### 替换流程

执行替换时, 会遍历目标`str`, 从 trie 树中查找出现的第一个 old 字符串并替换.

由于 oldnew 可能不只一对, 那么 old 也不只一对, 所以谁也不知道会从 str 中先找到哪个 old 串. 

如果按照常规的做法, 查找的流程可能涉及"回溯", 而 trie 树通过公共前缀简化了回溯流程, 这就是前缀树的意义.
