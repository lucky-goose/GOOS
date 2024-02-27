#include "syscall.h"
#include "print.h"

//无参数的系统调用
#define _syscall0(NUMBER) ({	\
	int rtnval;					\
	asm volatile (				\
		"int $0x80"				\
		: "=a" (rtnval)			\
		: "a" (NUMBER)			\
		:"memory"				\
	);							\
	rtnval;						\
})


//一个参数的系统调用
#define _syscall1(NUMBER, ARG1) ({ 	\
	int rtnval;						\
	asm volatile (					\
		"int $0x80"					\
		: "=a" (rtnval)				\
		: "a" (NUMBER), "b" (ARG1)	\
		:"memory"					\
	);								\
	rtnval;							\
})

//两个参数的系统调用
#define _syscall2(NUMBER, ARG1, ARG2)({			\
	int rtnval;									\
	asm volatile (								\
		"int $0x80"								\
		: "=a" (rtnval)							\
		: "a" (NUMBER), "b" (ARG1), "c" (ARG2)	\
		:"memory"								\
	);											\
	rtnval;										\
})

//三个参数的系统调用
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({				\
	int rtnval;												\
	asm volatile (											\
		"int $0x80"											\
		: "=a" (rtnval)										\
		: "a" (NUMBER), "b" (ARG1), "c" (ARG2), "d" (ARG3)	\
		:"memory"											\
	);														\
	rtnval;													\
})

//返回当前任务的pid
uint32_t getpid(void) {
	return _syscall0(SYS_GETPID);	
}

//打印字符串str
uint32_t write(char* str) {
	return _syscall1(SYS_WRITE, str);
}

//获取size大小的内存
void* malloc(uint32_t size) {
	return (void*)_syscall1(SYS_MALLOC, size);
}

//使用ptr指向的内存
void free(void* ptr) {
	_syscall1(SYS_FREE, ptr);
}
