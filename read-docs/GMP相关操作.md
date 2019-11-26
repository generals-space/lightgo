参考文章

1. [自己对go协程的理解](https://www.jianshu.com/p/4267cfbbc2d1)
    - qq群"天天"大神写的, 比较亲民
2. [go语言调度器源代码情景分析](https://mp.weixin.qq.com/mp/homepage?sn=8fc2b63f53559bc0cee292ce629c4788&__biz=MzU1OTg5NDkzOA%3D%3D&scene=18&hid=1&devicetype=iOS13.1.2&version=17000831&lang=zh_CN&nettype=WIFI&ascene=0&session_us=gh_ceeb25947b8b&fontScale=100&pass_ticket=CiGLC18BKjQ8hvKyiMSivuT%2ByXVZrOOaysEtqZt15G6c55gdqBGw7H11c4lzLcEG&wx_header=1&scene=1)
    - 微信系列文章

## M <=> P

acquirep(p): 将当前m与p绑定, m-p = p; p->m = m

releasep(): 将当前m与其绑定的p解绑, m->p = nil; p->m = nil

## 

runtime·malg: 创建新的g对象, 并为其分配初始栈空间.

runtime·newproc1(fn): 调用`runtime·malg`创建新的g对象, 来执行fn函数
