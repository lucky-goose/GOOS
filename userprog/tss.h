#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H

#include "thread.h"
#include "stdint.h"

//任务状态段tss结构
struct tss {
	uint32_t backlink;
	uint32_t* esp0;
	uint32_t ss0;
	uint32_t* esp1;
	uint32_t ss1;
	uint32_t* esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t (*eip) (void);
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint32_t trace;
	uint32_t io_base;
};

//所有进程公用的tss
static struct tss tss;

//更新tss中的esp0字段的值为pthread的0级栈
void update_tss_esp(struct task_struct* pthread);
//创建gdt描述符
struct gdt_desc make_gdt_desc(uint32_t* desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high);
//在gdt中创建tss并重新加载gdt
void tss_init(void);

#endif
