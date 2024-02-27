#include "stdio.h"
#include "syscall.h"
#include "string.h"

#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL

//将整型转换成字符串
void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base) {
	//求模
	uint32_t m = value % base;
	//取整
	uint32_t i = value / base;
	
	//如果不为0则递归调用
	if (i) itoa(i, buf_ptr_addr, base);
	
	//根据大小将其转换为字符
	if (m < 10) {
		**buf_ptr_addr = m + '0';
	} else {
		**buf_ptr_addr = m - 10 + 'A';
	}
	(*buf_ptr_addr)++;
}

//将参数ap按照格式format输出到字符串str，并返回替换后的str长度
uint32_t vsprintf(char* str, const char* format, va_list ap) {
	char* dst = str;
	const char* src = format;
	
	//用于临时存储转换后的变量
	int32_t arg_int;
	char* arg_str;
	//循环直至'\0'
	while (*src) {
		if (*src != '%') {
			*dst = *src;
			dst++;
			src++;
			continue;
		}
		//跳过%
		src++;
		switch (*src) {
			case 'x':
				arg_int = va_arg(ap, int);
				itoa(arg_int, &dst, 16);
				break;
			
			case 's':
				arg_str = va_arg(ap, char*);
				strcpy(dst, arg_str);
				dst += strlen(arg_str);
				break;
			
			case 'c':
				*dst = va_arg(ap, char);
				dst++;
				break;
				
			case 'd':
				arg_int = va_arg(ap, int);
				if (arg_int < 0) {
					*dst = '-';
					dst++;
					arg_int = 0 - arg_int;
				}
				itoa(arg_int, &dst, 10);
				break;
				
		} 
		//继续下一个字符
		src++;
	}
	return strlen(str);
}

//格式化输出字符串format
uint32_t printf(const char* format, ...) {
	//创建并初始化va_list指针
	va_list args;
	va_start(args, format);
	
	char buf[1024] = {0};
	//调用解析函数
	vsprintf(buf, format, args);
	va_end(args);
	//调用系统调用，并返回写了多少个字符
	return write(buf);
}

//将format写入到指定的buf中
uint32_t sprintf(char* buf, const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	uint32_t len = 0;
	len = vsprintf(buf, format, args);
	
	va_end(args);
	return len;
}
