#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H
#include "stdint.h"

//GDT描述符属性
#define DESC_G_4K 	1
#define DESC_D_32 	1
#define DESC_L		0
#define DESC_AVL	0
#define DESC_P		1
#define DESC_DPL_0	0
#define DESC_DPL_1	1
#define DESC_DPL_2	2
#define DESC_DPL_3	3

//代码段和数据段属于存储段，tss和各种门描述符属于系统段
//s为1表示存储段，为0表示系统段
#define DESC_S_CODE 	1
#define DESC_S_DATA 	DESC_S_CODE
#define DESC_S_SYS 		0

//x=1,c=0,r=0,a=0代码段是可执行的，非一致性，不可读，已访问位a清0
#define DESC_TYPE_CODE 	8
//x=0,e=0,w=1,a=0数据段是不可执行的，向上扩展的，可读，已访问位清0
#define DESC_TYPE_DATA	2
#define DESC_TYPE_TSS	9 


//选择子相关属性
#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

//段选择子
#define SELECTOR_K_CODE 	((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA 	((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK 	SELECTOR_K_DATA
#define SELECTOR_K_GS 		((3 << 3) + (TI_GDT << 2) + RPL0)
//第三个段描述符是显存，第4个是tss
#define SELECTOR_U_CODE 	((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA 	((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK 	SELECTOR_U_DATA

#define GDT_ATTR_HIGH ((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))

#define GDT_CODE_ATTR_LOW_DPL3 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE)

#define GDT_DATA_ATTR_LOW_DPL3 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_DATA)

//IDT描述符属性
#define IDT_DESC_P 	1
#define IDT_DESC_DPL0 	0
#define IDT_DESC_DPL3 	3
#define IDT_DESC_32_TYPE 0xE	//32位的门
#define IDT_DESC_16_TYPE 0x6	//16位的门

#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)

//PIC相关属性
#define PIC_M_CTRL 0x20	//主片控制端口
#define PIC_M_DATA 0x21	//主片数据端口

#define PIC_S_CTRL 0xa0	//从片控制端口
#define PIC_S_DATA 0xa1	//从片数据端口

//内存管理相关属性
#define PG_SIZE 4096
//位图地址安排在0xc009a00
//内核主线程pcb在0xc009e000
//内核主线从栈顶在0xc009f000
#define MEM_BITMAP_BASE 0xc009a000
//内核堆的起始地址在1MB之上
#define K_HEAP_START 0xc0100000

//页表相关属性
#define PG_P_1 	1	//存在
#define PG_P_0	0	//不存在
#define PG_RW_R 0	//读/执行
#define PG_RW_W 2	//读写/执行
#define PG_US_S 0	//系统级
#define PG_US_U 4	//用户级

//TSS描述符属性
#define TSS_DESC_D 0
#define TSS_ATTR_HIGH ((DESC_G_4K << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x0)

#define TSS_ATTR_LOW ((DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS)

#define SELECTOR_TSS ((4 << 3) + (TI_GDT << 2) + RPL0)

struct gdt_desc {
	uint16_t limit_low_word;
	uint16_t base_low_word;
	uint8_t base_mid_byte;
	uint8_t attr_low_byte;
	uint8_t limit_high_attr_high;
	uint8_t base_high_byte;
};

#define EFLAGS_MBS 	(1 << 1)
#define EFLAGS_IF_1	(1 << 9)
#define EFLAGS_IF_0	0
//IOPL3,用于测试用户程序在非系统调用下进行IO
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IOPL_0 (0 << 12)
//除法向上取整
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

#endif
