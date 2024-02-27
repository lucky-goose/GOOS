#include "sync.h"
#include "debug.h"
#include "interrupt.h"
#include "thread.h"

//初始化信号量
void sema_init(struct semaphore* psema, uint8_t value) {
	//为信号量赋值
	psema->value = value;
	//初始化等待队列
	list_init(&psema->waiters);
}

//初始化锁
void lock_init(struct lock* plock) {
	plock->holder = NULL;
	plock->holder_repeat_cnt = 0;
	sema_init(&plock->semaphore, 1);
}

//信号量减少，P操作
void sema_down(struct semaphore* psema) {
	//关中断来保证原子操作
	enum intr_status old_status = intr_disable();
	
	while (psema->value == 0) {
		ASSERT(!node_find(&psema->waiters, &running_thread()->general_tag));
		if (node_find(&psema->waiters, &running_thread()->general_tag)) {
			PANIC("sema_down:thread has been in waiters_list\n");
		}
		//如果信号量为0则阻塞线程
		list_append(&psema->waiters, &running_thread()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	
	//当value为1或被唤醒后
	psema->value--;
	ASSERT(psema->value == 0);
	//恢复中断状态
	set_intr_status(old_status);
}

//信号量增加，V操作
void sema_up(struct semaphore* psema) {
	//关中断来保证原子操作
	enum intr_status old_status = intr_disable();
	
	ASSERT(psema->value == 0);
	//如果等待队列不为空，则将第一个结点弹出
	if (!list_empty(&psema->waiters)) {
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;
	ASSERT(psema->value == 1);
	
	//恢复中断状态
	set_intr_status(old_status);
}

//获得锁
void lock_acquire(struct lock* plock) {
	//排除自己获得锁但未释放的情况
	if (plock->holder != running_thread()) {
		sema_down(&plock->semaphore);
		plock->holder = running_thread();
		ASSERT(plock->holder_repeat_cnt == 0);
		plock->holder_repeat_cnt = 1;
	} else {
		//自己重复获得锁
		plock->holder_repeat_cnt++;
	}
}

//释放锁
void lock_release(struct lock* plock) {
	ASSERT(plock->holder == running_thread());
	//如果多次获得锁则返回
	if (plock->holder_repeat_cnt > 1) {
		plock->holder_repeat_cnt--;
		return;
	}
	ASSERT(plock->holder_repeat_cnt == 1);
	
	//重置锁并使信号量加1
	plock->holder = NULL;
	plock->holder_repeat_cnt = 0;
	sema_up(&plock->semaphore);
}
