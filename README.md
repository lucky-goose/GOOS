# GOOS
一个玩具os
# 具体功能：
- 1：实现了BootLoader，从进入保护模式到启动分页机制，最后加载内核
- 2：实现了内存管理系统，涵盖从获取物理页到实现malloc
- 3：实现了进程线程机制，支持任务调度，应用了时间片轮转算法
- 4：实现了中断机制，模仿linux使用中断门实现了系统调用
- 5：实现了并发机制，用二元信号量配合中断实现了锁
- 6：实现了输入输出系统，编写了键盘驱动，支持从键盘获取输入
# 文件夹说明
- 1：boot文件夹中存放与bootloader相关的文件
- 2：device文件夹中存放与外设相关的文件
- 3：kernel文件夹中存放与内核初始化和运行相关的文件
- 4：lib文件夹中存放定义工具函数的相关文件
- 5：thread文件夹中存放与实现线程相关文件
- 6：userprog文件夹中存放与实现用户进程相关文件
