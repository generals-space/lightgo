## M <=> P

acquirep(p): 将当前m与p绑定, m-p = p; p->m = m

releasep(): 将当前m与其绑定的p解绑, m->p = nil; p->m = nil

## 

runtime·malg: 创建新的g对象, 并为其分配初始栈空间.

runtime·newproc1(fn): 调用`runtime·malg`创建新的g对象, 来执行fn函数
