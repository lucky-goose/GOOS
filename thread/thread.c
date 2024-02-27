#include "thread.h"
#include "string.h"
#include "global.h"
#include "interrupt.h"
#include "debug.h"
#include "bitmap.h"
#include "process.h"
#include "print.h"

//由kernel_thread去执行function(args)
void kernel_thread(thread_func* function, void* func_args) {
	//执行线程前要开启中断，避免后面的时钟中断被屏蔽，而无法调度其他线程
	intr_enable();
	function(func_args);
}

//初始化线程栈
void thread_create(struct task_struct* pthread, thread_func function, void* func_args) {
	pthread->self_kstack -= sizeof(struct intr_stack);
	
	pthread->self_kstack -= sizeof(struct thread_stack);
	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_args = func_args;
	kthread_stack->ebp = 0;
	kthread_stack->ebx = 0;
	kthread_stack->esi = 0;
	kthread_stack->edi = 0;
}

//初始化线程基本信息PCB
void init_thread(struct task_struct* pthread, char* name, uint8_t priority) {
	//将PCB清0
	memset(pthread, 0, sizeof(*pthread));
	
	if (pthread == main_thread) {
		pthread->status = TASK_RUNNING;
	} else {
		pthread->status = TASK_READY;
	}

	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->pid = allocate_pid();
	pthread->priority = priority;
	pthread->ticks = priority;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	strcpy(pthread->name, name);
	//自定义魔数
	pthread->stack_magic = 0x20030621;
}

//分配pid
pid_t allocate_pid(void) {
	static pid_t next_pid = 0;
	lock_acquire(&pid_lock);
	next_pid++;
	lock_release(&pid_lock);
	return next_pid;
}

//创建线程并执行
struct task_struct* thread_start(char* name, uint8_t priority, thread_func function, void* func_args) {
	//pcb都在内核空间
	struct task_struct* thread = get_kernel_pages(1);
	
	init_thread(thread, name, priority);
	thread_create(thread, function, func_args);
	
	//确保之前不再队列中
	ASSERT(!node_find(&thread_ready_list, &thread->general_tag));
	//加入就绪线程队列
	list_append(&thread_ready_list, &thread->general_tag);
	
	//确保之前不再队列中
	ASSERT(!node_find(&thread_all_list, &thread->all_list_tag));
	//加入全部线程队列
	list_append(&thread_all_list, &thread->all_list_tag);
	
	return thread;
}

//获取当前线程的pcb指针
struct task_struct* running_thread(void) {
	uint32_t esp;
	asm volatile ("mov %%esp, %0" : "=g" (esp));
	//取esp整数部分，即pcb的起始地址
	return (struct task_struct*)(esp & 0xfffff000);
}

//将kernel中的main函数完善为主线程  
void make_main_thread(void) {
	//在loaders.S进入内核时的mov esp, 0xc009f000
	//就是为main预留的pcb
	//故不需要再另外分配一页
	main_thread = running_thread();
	init_thread(main_thread, "main", 31);
	
	//main_thread是当前线程，所以将其加入到thread_all_list中
	//确保之前不再队列中
	ASSERT(!node_find(&thread_all_list, &main_thread->all_list_tag));
	//加入全部线程队列
	list_append(&thread_all_list, &main_thread->all_list_tag);
}

//实现任务调度
void schedule(void) {
	ASSERT(get_intr_status() == INTR_OFF);
	
	struct task_struct* cur = running_thread();
	if (cur->status == TASK_RUNNING) {
		//将当前线程加入队列
		ASSERT(!node_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	} else {
		//若当前线程需要某事发生后才能继续上cpu运行，那么不需要加入就绪队列
	}
	
	//获取下一个待运行的线程，并切换到cpu上
	ASSERT(!list_empty(&thread_ready_list));
	thread_tag == NULL;
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
	next->status = TASK_RUNNING;
	
	//激活下一个任务的页表
	process_activate(next);
	
	switch_to(cur, next);
}

//初始化线程环境
void thread_init(void) {
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	lock_init(&pid_lock);
	make_main_thread();
	put_str("thread_init done\n");
}

//阻塞当前线程
void thread_block(enum task_status stat) {
	//stat取值为以下三种之一才能阻塞线程
	ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING));
	
	//关中断，获取当前正在运行的线程
	enum intr_status old_status = intr_disable();
	struct task_struct* cur_thread = running_thread();
	//更改线程状态并调度下一个线程
	cur_thread->status = stat;
	schedule();
	//线程解阻塞后才能执行下面这句
	set_intr_status(old_status);
}

//将pthread线程解阻塞
void thread_unblock(struct task_struct* pthread) {
	//关中断
	enum intr_status old_status = intr_disable();
	
	ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));
	
	if (pthread->status != TASK_READY) {
		ASSERT(!node_find(&thread_ready_list, &pthread->general_tag));
		
		if (node_find(&thread_ready_list, &pthread->general_tag)) {
			PANIC("thread is in ready_list\n");
		}
		//将阻塞的进程放在队首，尽快调度
		list_push(&thread_ready_list, &pthread->general_tag);
		pthread->status = TASK_READY;
	} 
	set_intr_status(old_status);
}
