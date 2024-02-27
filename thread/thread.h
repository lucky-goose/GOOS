#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"

//自定义函数类型，用于在线程函数中作为形参类型
typedef void thread_func(void*);

typedef int16_t pid_t;

//进程或线程的状态
enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

//中断栈，用于发生中断时保护程序的上下文，此栈在内核栈所在页的顶端
struct intr_stack {
	//中断号
	uint32_t vec_no;
	//寄存器现场
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	
	//当发生特权级转移时压入
	uint32_t err_code;
	void (*eip)(void);
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss; 
};

//线程栈，用于存储线程中待执行的函数
//位置不固定，用于switch_to时保存线程环境
struct thread_stack {
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	
	//线程第一次执行时，eip指向待调用的函数kernel_thread
	//其他时候，eip是指向switch_to的返回地址
	void (*eip)(thread_func* func, void* func_args);
	
	//以下仅供第一次被调度上cpu时使用
	
	//返回地址，占位
	void (*unused_rtnaddr);
	//由kernel_thread调用的函数名
	thread_func* function;
	//由kernel_thread调用的函数的参数
	void* func_args;
};

//进程的PCB,位于内核栈所在页的低端
struct task_struct {
	//内核栈的地址
	uint32_t* self_kstack;
	//进程号
	pid_t pid;
	//线程状态
	enum task_status status;
	//线程名
	char name[16];
	//线程的优先级
	uint8_t priority;
	//每次在处理器上执行的滴答数
	uint8_t ticks;
	//此任务上cpu后执行的总滴答数
	uint32_t elapsed_ticks;
	//用于在一般的thread_ready_list中
	struct list_node general_tag;
	//用于在thrad_all_list中
	struct list_node all_list_tag;
	//进程自己的页表地址
	uint32_t* pgdir;
	//用户进程的虚拟地址
	struct virtual_addr_pool userprog_vaddr_pool;
	//用户进程内存块描述符
	struct mem_block_desc u_block_descs[MEM_BLOCK_DESC_CNT];
	//魔数，用于检测栈的溢出
	uint32_t stack_magic;
};

//主线程
struct task_struct* main_thread;
//就绪队列
struct list thread_ready_list;
//所有线程队列
struct list thread_all_list;
//临时变量
struct list_node* thread_tag;
//获取pid用的锁
struct lock pid_lock;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

//由kernel_thread去执行function(args);
void kernel_thread(thread_func* function, void* func_args);
//初始化线程栈
void thread_create(struct task_struct* pthread, thread_func function, void* func_args);
//初始化线程基本信息PCB
void init_thread(struct task_struct* pthread, char* name, uint8_t priority);
//分配pid
pid_t allocate_pid(void);
//创建线程并执行
struct task_struct* thread_start(char* name, uint8_t priority, thread_func function, void* func_args);
//获取当前线程的pcb指针
struct task_struct* running_thread(void);
//将kernel中的main函数完善为主线程  
void make_main_thread(void);
//实现任务调度
void schedule(void);
//初始化线程环境
void thread_init(void);
//阻塞当前线程
void thread_block(enum task_status stat);
//将pthread线程解阻塞
void thread_unblock(struct task_struct* pthread);

#endif

