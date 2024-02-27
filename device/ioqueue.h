#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

//环形队列
struct ioqueue {
	//生产者消费者问题
	struct lock lock;
	//此项用于记录哪个生产者在此缓冲区上睡眠
	struct task_struct* producer;
	//此项用于记录哪个消费者在此缓冲区上睡眠
	struct task_struct* consumer;
	//缓冲区
	uint8_t buf[bufsize];
	//队首
	int32_t head;
	//队尾
	int32_t tail;
};

//初始化io队列
void ioqueue_init(struct ioqueue* ioq);
//返回pos在缓冲区的下一个位置
int32_t next_pos(uint32_t pos);
//判断队列是否已满
bool ioq_full(struct ioqueue* ioq);
//判断队列是否已空
bool ioq_empty(struct ioqueue* ioq);
//使当前生产者或消费者在缓冲区上等待
void ioq_wait(struct task_struct** waiter);
//唤醒waiter
void wakeup(struct task_struct** waiter);
//消费者从ioq队列中获取一个字符
uint8_t ioq_getchar(struct ioqueue* ioq);
//生产者往ioq队列中写入一个字符
void ioq_putchar(struct ioqueue* ioq, uint8_t byte);
#endif
