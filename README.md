## simplec 简介

    Now version 2.0.2 release
    Best new gcc and cl compile successfully, Zero warnning!

![simplec title image hello ya!](http://images.cnblogs.com/cnblogs_com/life2refuel/1022495/o_simplec.jpg)

    这个库是 C 跨平台的简单的基础库.(搭建框架层的小型底层库)
    非常的 simple, 高效. 特别适合喜欢 C 的和学有余力的在校生可以的学习和参考. 
    
    目前具备的功能如下:
    
    1) extern - 通用外部库(希望尽可能的少)
        a) pthread      : POSIX线程库, winds 采用的是 GerHobbelt/pthread-win32 最新版本单独升级编译
        b) libcurl      : http 库, 下载类最新源码编译, winds and linux
    
    2) module - 内部功能
        a) 单元测试, 可以参照 simplec 和 test 文件夹下
        b) 精简高效的日志 clog 搭配 logrotate
        c) 精简的配置解析库 scconf.h
        d) 快速的 scjson.h 解析引擎, 比 cJSON 库好. 序列化方案, 自己插入
        e) 跨平台的网络io, 基于 select + epoll + kevent
        f) 内存监测, csv解析, 自旋锁, 读写锁, 时间业务, 中文加解码, 协程库...
        g) 常用数据结构, string, array, tree, rbtree, list, queue....
        ... ...
    
    ヽ(^_−)ﾉ  一切才刚刚开始 ~~

### winds 搭建

    这个项目目前在 Visual Stduio 2013/2015/2017, 都搞过. 推荐 Best new Visual Stdio CL
    一开始支持 x86 and x64, 为了后期维护方便, 目前仅支持x86(以后x86平台会退出历史的舞台). 
    详细的环境搭建可以查看 /simplec/readme/helper.txt 文档搭建 VS 开发环境配置. 

### linux 搭建

    目前是 Best New Ubuntu and GCC 搭建开发的. 
    (Makefile 可能更新不是那么及时, 每发布一个合适的版本, 会统一构建一次 make)
    
    一般编译步骤, 首次需要安装 libcurl, libssl 等库
```bash

a) libcurl

sudo apt-get install libcurl4-openssl-dev

```

    后面使用编译直接 make.
    
    更加详细的编译流程参照 Makefile and /simplec/readme/helper.txt

### 后记

    希望遇到同好这口的人, 哈哈. 欢迎 push.
    simplec ヾ(⌐ ■_■)

```C
    $ printf 年华虚度，空有一身疲倦
    
    <<也许>> 
    - 汪国真
    也许，永远没有那一天
    前程如朝霞般绚烂
    也许，永远没有那一天
    成功如灯火般辉煌
    也许，只能是这样
    攀援却达不到峰顶
    也许，只能是这样
    奔流却掀不起波浪
    也许，我们能给予你的
    只有一颗
    饱经沧桑的心
    和满脸风霜
```

#### 展望

    simplec 框架基本都搭建完毕了, 关于 C 基础开发部分. 但对于自己目前而言
    (2017年10月5日22:14:42) 非常的不满意! 我们解决问题的宗旨, 是最简单, 但是
    发现封装之后, 变得更加杂合!(静态库和动态库的纠缠 ~)

    当然, 可以通过更好代码结构. 清理不常用的模块, 框架布局的调整. 重构很多写的很糙库, 
    壮实单元测试 ....... 设计理念 ......

    但, 说真的, 真的有必要吗 哈哈, (但我会回来, 造个满意的扳手.) 至少 C 语言之父, 
    有的走了, 有的在 go 路上 o(╥﹏╥)o

    希望下次能写个更好的 base c, 而不是这个槽糕的乱炖.

    最后扯一点, C 特别的灵活, 喜欢的人喜欢的不得了. 但, 如果你是个新手, 并且不是三好
    学生. 只是很感兴趣, 推荐不要入坑(有那时间倒不如看看算法, 或者跑跑步). 

    下个目标, 充分熟练运用 go, 再回来为下一个 base c 开路, 当然还需要极大提升三大
    神功, 数据结构, 操作系统, 算法 

***
    胜   在   轮   回

    人生如戏，妆颜上戏台，一颦一笑由己更由命；

    世事如棋，提子入棋局，一纵一横由人更由天。

