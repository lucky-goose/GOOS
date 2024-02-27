#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"

typedef void* intr_handler;
//中断门描述符
struct gate_desc {
	uint16_t func_offset_low_word;	//偏移量的低16位
	uint16_t selector;				//段选择子
	uint8_t dcount;					//此项固定
	uint8_t attribute;				
	uint16_t func_offset_high_word;	//偏移量的高16位
};

//定义中断的两种状态，关闭（0）和开启（1）
enum intr_status{
	INTR_OFF,
	INTR_ON
};

void init_idt_desc(struct gate_desc* g_desc, uint8_t attr, intr_handler function);
void init_idt(void);
void init_pic(void);
void init_intr(void);
void general_intr_handler(uint8_t vec_index);
void init_exception(void);
void register_handler(uint8_t intr_num, intr_handler function);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
enum intr_status set_intr_status(enum intr_status status);
enum intr_status get_intr_status(void);
#endif
