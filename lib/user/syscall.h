#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR {
	SYS_GETPID,
	SYS_WRITE,
	SYS_MALLOC,
	SYS_FREE
};

//返回当前任务的pid
uint32_t getpid(void);
//打印字符串str
uint32_t write(char* str);
//获取size大小的内存
void* malloc(uint32_t size);
//使用ptr指向的内存
void free(void* ptr);

#endif
