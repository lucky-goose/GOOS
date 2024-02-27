#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"

#define IRQ0_FREQUENCY 		100
#define INPUT_FREQUENCY		1193180
#define COUNTER0_VALUE 		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 		0X40
#define COUNTER0_NO 		0
#define COUNTER0_MODE 		2
#define READ_WRITE_LATCH 	3
#define PIT_CONTROL_PORT	0x43

//设置计数器频率
void set_frequency(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value) {
	//往控制字寄存器写入控制字
	outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
	//写入counter_value的低8位
	outb(counter_port, (uint8_t)counter_value);
	//写入counter_value的高8位
	outb(counter_port, (uint8_t)(counter_value >> 8));
} 

//初始化PIT8253
void init_timer() {
	put_str("timer_init start\n");
	set_frequency(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE, COUNTER0_VALUE);
	//注册中断处理函数
	register_handler(0x20, intr_timer_handler);
	put_str("timer_init done\n");
}

void intr_timer_handler(void) {
	struct task_struct* cur_thread = running_thread();
	
	//检查栈是否溢出
	ASSERT(cur_thread->stack_magic == 0x20030621);
	
	//当前进程总时间数+1
	cur_thread->elapsed_ticks++;
	//总的时钟中断数+1
	ticks++;
	
	//时间片用完了则调度新的进程
	if (cur_thread->ticks == 0) {
		schedule();
	} else {
		cur_thread->ticks--;
	}
}
