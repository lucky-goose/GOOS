#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "stdint.h"
#include "thread.h"

#define USER_VADDR_START 0x8048000
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define DEFAULT_PRIO 31

//构建用户进程初始上下文信息
void start_process(void* filename);
//激活页表
void page_dir_activate(struct task_struct* p_thread);
//激活线程或进程的页表，更新tss中esp0为进程的特权级0的栈
void process_activate(struct task_struct* p_thread);
//创建页目录表，将当前页表的表示内核的pde复制，成功则返回页表的虚拟地址，否则返回-1
uint32_t* create_page_dir(void);
//创建用户进程虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct* user_prog);
//创建用户进程
void process_execute(void* filename, char* name);

#endif
