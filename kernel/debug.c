#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(char* filename, int line, const char* func, const char* condition){
	//先关闭中断
	intr_disable();
	
	put_str("\n!!!error!!!\n");
	
	//打印文件名
	put_str("filename:");
	put_str(filename);
	put_str("\n");

	//打印行号
	put_str("line:0x");
	put_int(line);
	put_str("\n");
	
	//打印函数名
	put_str("function:");
	put_str((char*)func);
	put_str("\n");
	
	//打印断言函数
	put_str("condition:");
	put_str((char*)condition);
	put_str("\n");
	while(1);
}
