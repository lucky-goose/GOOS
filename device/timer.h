#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_h
#include "stdint.h"
//从开中断以来总共的滴答数
uint32_t ticks;

void set_frequency(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value);
void init_timer(void); 
//时钟中断处理函数
void intr_timer_handler(void);
#endif
