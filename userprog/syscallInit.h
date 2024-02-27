#ifndef __USERPROG_SYSCALLINIT_H
#define __USERPROG_SYSCALLINIT_H
#include "stdint.h"

#define syscall_cnt 32
typedef void* syscall;
syscall syscall_table[syscall_cnt];

//返回当前任务的pid
uint32_t sys_getpid(void);
//打印字符串str(未实现文件系统前的版本）
uint32_t sys_write(char* str);
//初始化系统调用
void syscall_init(void);


#endif
