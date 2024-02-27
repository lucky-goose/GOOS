#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include "stdint.h"

typedef char* va_list;

//将整型转换成字符串
void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base);
//将参数ap按照格式format输出到字符串str，并返回替换后的str长度
uint32_t vsprintf(char* str, const char* format, va_list ap);
//格式化输出字符串format
uint32_t printf(const char* format, ...);
//将format写入到指定的buf中
uint32_t sprintf(char* buf, const char* format, ...);

#endif
