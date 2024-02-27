#include "ioqueue.h"
#include "debug.h"
#include "interrupt.h"

//初始化io队列
void ioqueue_init(struct ioqueue* ioq) {
	lock_init(&ioq->lock);
	ioq->producer = NULL;
	ioq->consumer = NULL;
	ioq->head = 0;
	ioq->tail = 0;
}

//返回pos在缓冲区的下一个位置
int32_t next_pos(uint32_t pos) {
	return (pos + 1) % bufsize;
}

//判断队列是否已满
bool ioq_full(struct ioqueue* ioq) {
	ASSERT(get_intr_status() == INTR_OFF);
	return next_pos(ioq->head) == ioq->tail; 
}

//判断队列是否已空
bool ioq_empty(struct ioqueue* ioq) {
	ASSERT(get_intr_status() == INTR_OFF);
	return ioq->head == ioq->tail;
}

//使当前生产者或消费者在缓冲区上等待
void ioq_wait(struct task_struct** waiter) {
	ASSERT(*waiter == NULL && waiter != NULL);
	*waiter = running_thread();
	thread_block(TASK_BLOCKED);
}

//唤醒waiter
void wakeup(struct task_struct** waiter) {
	ASSERT(*waiter != NULL);
	thread_unblock(*waiter);
	*waiter = NULL;
}

//消费者从ioq队列中获取一个字符
uint8_t ioq_getchar(struct ioqueue* ioq) {
	//确保在关中断情况下运行
	ASSERT(get_intr_status() == INTR_OFF);
	
	//如果队列为空，消费者就阻塞在缓冲区上
	//目的是让生产者知道要通知哪个消费者
	while (ioq_empty(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->consumer);
		lock_release(&ioq->lock);
	}
	
	//获取一字节的数据
	uint8_t byte = ioq->buf[ioq->tail];
	ioq->tail = next_pos(ioq->tail);
	
	//唤醒生产者
	if (ioq->producer != NULL) {
		wakeup(&ioq->producer);
	}
	
	return byte;
}

//生产者往ioq队列中写入一个字符
void ioq_putchar(struct ioqueue* ioq, uint8_t byte) {
	//确保在关中断情况下运行
	ASSERT(get_intr_status() == INTR_OFF);
	
	//如果队列满，生产者就阻塞在缓冲区上
	//目的是让消费者知道要唤醒哪个生产者
	while (ioq_full(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->producer);
		lock_release(&ioq->lock);
	}
	
	//往缓冲区写入数据
	ioq->buf[ioq->head] = byte;
	ioq->head = next_pos(ioq->head);
	
	//唤醒消费者
	if (ioq->consumer != NULL) {
		wakeup(&ioq->consumer);
	}
}
