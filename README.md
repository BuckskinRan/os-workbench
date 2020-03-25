# 1. os-workbench
lpr learning OS in NJU
<!-- TOC -->

- [1. os-workbench](#1-os-workbench)
    - [1.1. M0](#11-m0)
    - [1.2. M1](#12-m1)
    - [1.3. L0](#13-l0)

<!-- /TOC -->

## 1.1. M0
- 打印时候建立树后一次深度优先搜索即可，-n对程序的影响要高于-p，-p只影响打印格式，-n影响排序方式，一开始没有理清楚逻辑带来了一点点困惑
- 感谢某王姓同学的建议，需要考虑父节点PID反而小于字节点PID的情形
- -V的输出使用stderr，且优点度最高

## 1.2. M1
`现阶段感想，发现错误随时更改`

对于单独的某个进程而言，估计`yield()`在单个协程的运行看来实际效果如下：
```
+----------------+
|execute nomarly |
+----------------+
        |
        | 
        +->+----------+
           |call yield|
           +----------+
                 |
           +----------+
           |  set_jmp |
           +----------+
                 |
          +--------------+
          |do other thing|
          +--------------+
                 |
            +-----------+
            | come back | // 在这种情况下，别的协程顺利调用了这个协程的恢复部分，然后由于走之前又一次set_jmp, 这次用long_jmp跳回来，就可以区分了
            +-----------+
                 |
        +--------+
        |
+----------------+
|continue execute|   
+----------------+                  

``` 
OK, 那么现在的问题就是应该怎么恢复寄存器和堆栈现场，考虑这两个的先后顺序了，如果要先恢复寄存器，那么PC就会回到当初程序`set_jmp`的位置，这个时候似乎就会直接继续进行下一条指令了（）


`stack_switch_call`
```
   before                 switch stack              start of func
+---------+               +---------+                +---------+          
|         | <- old esp    |         |                |         |
+---------+               +---------+                +---------+
|         |               |         |                |         |
+---------+               +---------+                +---------+
    ...                       ...                        ...
+---------+               +---------+                +---------+
|         | <- old ebp    |         | <- old ebp     |         |
+---------+               +---------+                +---------+
    ...                       ...                        ...
+---------+               +---------+                +---------+
|    co   |               |   co    |                |   co    |
+---------+               +---------+                +---------+
    ...                       ...                        ...
+---------+               +---------+                +---------+
| co stack|               | co stack|                | co stack|
+---------+               +---------+                +---------+
    ...                       ...                        ...
+---------+               +---------+                +---------+
|st bottom|               |st bottom|                | old ebp | <- new esp, new ebp
+---------+               +---------+                +---------+
|  co + 1 |               |  co + 1 | <- new esp     |  co + 1 | 
+---------+               +---------+                +---------+
```
---


## 1.3. L0
- 写2048的时候没有注意到色号不符合VGA的格式，搞得程序老是崩溃，自己还没有找到问题
- 绘制方块参考老师splash里的方式即可，splash里的方块确实小了点，可以调整绘制时候x，y值（比如使用x / 3,y / 3就可以让边长变成原来打三倍），不建议调整SIDE，调整SIDE后出现过一些奇奇怪怪的问题，与同学讨论后没有结果