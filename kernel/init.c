#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscallInit.h"

void init_all() {
	put_str("init all start\n");
	//初始化中断
	init_intr();
	//初始化计数器
	init_timer();
	//初始化内存管理系统
	mem_init();
	//初始化线程环境
	thread_init();
	//初始化控制台
	console_init();
	//初始化键盘
	keyboard_init();
	//初始化进程状态段
	tss_init();
	//初始化系统调用
	syscall_init();
	
	put_str("init all done\n");
}
