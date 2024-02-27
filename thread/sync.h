#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"

//信号量结构体
struct semaphore {
	uint8_t value;
	struct list waiters;
};

struct lock {
	//锁的持有者
	struct task_struct* holder;
	//用二元信号量实现锁
	struct semaphore semaphore;
	//锁的持有者实现锁的次数
	uint32_t holder_repeat_cnt;
};

//初始化信号量
void sema_init(struct semaphore* psema, uint8_t value);
//初始化锁
void lock_init(struct lock* plock);
//信号量减少，P操作
void sema_down(struct semaphore* psema);
//信号量增加，V操作
void sema_up(struct semaphore* psema);
//获得锁
void lock_acquire(struct lock* plock);
//释放锁
void lock_release(struct lock* plock);
#endif
