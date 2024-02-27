#include "string.h"
#include "global.h"
#include "debug.h"

//将dst_开头的size个字节置为value
void memset(void* dst_, uint8_t value, uint32_t size) {
	ASSERT(dst_ != NULL);
	uint8_t* dst = (uint8_t*) dst_;
	while (size--) {
		*dst = value;
		dst++;
	}
}

//将src_起始的size个字节复制到dst_
void memcpy(void* dst_, const void* src_, uint32_t size) {
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	const uint8_t* src = (const uint8_t*)src_;
	while (size--) {
		*dst = *src;
		dst++;
		src++;
	}
}

//比较a_和b_开头的size个字节，若相等则返回0，a>b则返回1，a<b则返回-1
int memcmp(const void* a_, const void* b_, uint32_t size) {
	ASSERT(a_ != NULL && b_ != NULL);
	const char* a = a_;
	const char* b = b_;
	while (size--) {
		if (*a > *b) return 1;
		if (*a < *b) return -1;
		a++;
		b++;
	}
	return 0;
}

//将字符串从src_复制到dst_
char* strcpy(char* dst_, const char* src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
	char* rtn_value = dst_;
	while (*src_) {
		*dst_ = *src_;
		dst_++;
		src_++;
	}
	return rtn_value;
}

//返回字符串长度
uint32_t strlen(const char* str) {
	ASSERT(str != NULL);
	const char* p = str;
	while (*p++);
	return (p - str - 1);
}

//比较两个字服串a>b返回1，a=b返回0，a<b返回-1
int8_t strcmp(const char* a, const char* b) {
	ASSERT(a != NULL && b != NULL);
	while (a != 0 && *a == *b) {
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}

//从左到右查找字符串str中首次出现字符ch的地址
char* strchr(const char* str, const uint8_t ch) {
	ASSERT(str != NULL);
	while (*str != 0) {
		if (*str == ch) {
			return (char*)str;
		}
		str++;
	}
	return NULL;
}

//从右到右查找字符串str中首次出现字符ch的地址
char* strrchr(const char* str, const uint8_t ch) {
	ASSERT(str != NULL);
	const char* last_appear = NULL;
	while (*str != 0) {
		if (*str == ch) {
			last_appear = str;
		}
		str++;
	}
	return (char*)last_appear;
}

//把src_拼接到dst_后，返回拼接的串地址
char* strcat(char* dst_, const char* src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
	while (*dst_ != 0) dst_++;
	char* dst = dst_;
	while (*src_ != 0) {
		*dst = *src_;
		dst++;
		src_++;
	}
	*dst = 0;
	return dst_;
}

//在字符串str中查找字符ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch) {
	ASSERT(str != NULL);
	uint32_t cnt = 0;
	const char* p = str;
	while (*p != 0) {
		if (*p == ch) cnt++;
		p++;
	}
	return cnt;
}
