#ifndef __LIB_STRING_H
#define __LIB_STRING_H
#include "stdint.h"

//将dst_开头的size个字节置为value
void memset(void* dst_, uint8_t value, uint32_t size);
//将src_起始的size个字节复制到dst_
void memcpy(void* dst_, const void* src_, uint32_t size);
//比较a_和b_开头的size个字节，若相等则返回0，a>b则返回1，a<b则返回-1
int memcmp(const void* a_, const void* b_, uint32_t size); 
//将字符串从src_复制到dst_
char* strcpy(char* dst_, const char* src_);
//返回字符串长度
uint32_t strlen(const char* str);
//比较两个字服串a>b返回1，a=b返回0，a<b返回-1
int8_t strcmp(const char* a, const char* b);
//从左到右查找字符串str中首次出现字符ch的地址
char* strchr(const char* str, const uint8_t ch);
//从右到右查找字符串str中首次出现字符ch的地址
char* strrchr(const char* str, const uint8_t ch);
//把src_拼接到dst_后，返回拼接的串地址
char* strcat(char* dst_, const char* src_);
//在字符串str中查找字符ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch);

#endif

