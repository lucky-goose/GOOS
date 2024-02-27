[bits 32]
%define ERROR_CODE nop	;对于已压入错误码的，什么都不做
%define ZERO push 0	;对于没有错误码的，压入0占位

extern put_str

;c语言编写的中断处理程序数组
extern handlers

section .data
global intr_entry_table
intr_entry_table:

%macro VECTOR 2
section .text
;当发生中断时，cpu压入栈的寄存器：
;ss_old(若发生特权级变化） esp_old(若发生特权级变化） eflags cs_old eip_old error_code(若有）
;并且会自动关闭中断
intr%1entry:
	
	%2				;占位，以确保格式一致
	;因为中断处理程序是c语言编写，不确定会用到哪些寄存器
	;所以保存上下文环境
	push ds
	push es
	push fs
	push gs
	pushad
	
	mov al, 0x20	;0x20为中断结束命令EOI
	out 0xa0, al	;向从片发送
	out 0x20, al	;向主片发送
	
	;中断号入栈，方便调试
	push %1
	
	;调用c版本的中断处理函数
	call [handlers + %1 * 4]
	jmp intr_exit

section .data
	dd intr%1entry	;存储中断入口程序的地址，形成intr_entry_table数组

%endmacro

section .text
global intr_exit
intr_exit:
	;回收中断号
	add esp, 4
	
	;恢复上下文
	popad
	pop gs
	pop fs
	pop es
	pop ds
	
	;跳过错误码
	add esp, 4	
	;iret会恢复进入中断前保存的寄存器
	iret

VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ZERO
VECTOR 0x09, ZERO
VECTOR 0x0a, ZERO
VECTOR 0x0b, ZERO
VECTOR 0x0c, ZERO
VECTOR 0x0d, ZERO
VECTOR 0x0e, ZERO
VECTOR 0x0f, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ZERO
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO
VECTOR 0x19, ZERO
VECTOR 0x1a, ZERO
VECTOR 0x1b, ZERO
VECTOR 0x1c, ZERO
VECTOR 0x1d, ZERO
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO
VECTOR 0x20, ZERO	;时钟中断入口
VECTOR 0x21, ZERO	;键盘中断入口
VECTOR 0x22, ZERO	;级联用的
VECTOR 0x23, ZERO	;串口2对应的入口
VECTOR 0x24, ZERO	;串口1对应的入口
VECTOR 0x25, ZERO	;并口2对应的入口
VECTOR 0x26, ZERO	;软盘对应的入口
VECTOR 0x27, ZERO	;并口1对应的入口
VECTOR 0x28, ZERO	;实时时钟对应的入口
VECTOR 0x29, ZERO	;重定向
VECTOR 0x2a, ZERO	;保留
VECTOR 0x2b, ZERO	;保留
VECTOR 0x2c, ZERO	;ps/2鼠标
VECTOR 0x2d, ZERO	;fpu浮点单元异常
VECTOR 0x2e, ZERO	;硬盘
VECTOR 0x2f, ZERO	;保留
VECTOR 0x30, ZERO
VECTOR 0x31, ZERO
VECTOR 0x32, ZERO
VECTOR 0x33, ZERO
VECTOR 0x34, ZERO
VECTOR 0x35, ZERO
VECTOR 0x36, ZERO
VECTOR 0x37, ZERO
VECTOR 0x38, ZERO
VECTOR 0x39, ZERO
VECTOR 0x3a, ZERO
VECTOR 0x3b, ZERO
VECTOR 0x3c, ZERO
VECTOR 0x3d, ZERO
VECTOR 0x3e, ZERO
VECTOR 0x3f, ZERO
VECTOR 0x40, ZERO
VECTOR 0x41, ZERO
VECTOR 0x42, ZERO
VECTOR 0x43, ZERO
VECTOR 0x44, ZERO
VECTOR 0x45, ZERO
VECTOR 0x46, ZERO
VECTOR 0x47, ZERO
VECTOR 0x48, ZERO
VECTOR 0x49, ZERO
VECTOR 0x4a, ZERO
VECTOR 0x4b, ZERO
VECTOR 0x4c, ZERO
VECTOR 0x4d, ZERO
VECTOR 0x4e, ZERO
VECTOR 0x4f, ZERO
VECTOR 0x50, ZERO
VECTOR 0x51, ZERO
VECTOR 0x52, ZERO
VECTOR 0x53, ZERO
VECTOR 0x54, ZERO
VECTOR 0x55, ZERO
VECTOR 0x56, ZERO
VECTOR 0x57, ZERO
VECTOR 0x58, ZERO
VECTOR 0x59, ZERO
VECTOR 0x5a, ZERO
VECTOR 0x5b, ZERO
VECTOR 0x5c, ZERO
VECTOR 0x5d, ZERO
VECTOR 0x5e, ZERO
VECTOR 0x5f, ZERO
VECTOR 0x60, ZERO
VECTOR 0x61, ZERO
VECTOR 0x62, ZERO
VECTOR 0x63, ZERO
VECTOR 0x64, ZERO
VECTOR 0x65, ZERO
VECTOR 0x66, ZERO
VECTOR 0x67, ZERO
VECTOR 0x68, ZERO
VECTOR 0x69, ZERO
VECTOR 0x6a, ZERO
VECTOR 0x6b, ZERO
VECTOR 0x6c, ZERO
VECTOR 0x6d, ZERO
VECTOR 0x6e, ZERO
VECTOR 0x6f, ZERO
VECTOR 0x70, ZERO
VECTOR 0x71, ZERO
VECTOR 0x72, ZERO
VECTOR 0x73, ZERO
VECTOR 0x74, ZERO
VECTOR 0x75, ZERO
VECTOR 0x76, ZERO
VECTOR 0x77, ZERO
VECTOR 0x78, ZERO
VECTOR 0x79, ZERO
VECTOR 0x7a, ZERO
VECTOR 0x7b, ZERO
VECTOR 0x7c, ZERO
VECTOR 0x7d, ZERO
VECTOR 0x7e, ZERO
VECTOR 0x7f, ZERO
VECTOR 0x80, ZERO

;0x80号中断处理程序
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
;第一步：保存上下文环境
	push 0	;压入error_code，使栈中格式一致
	
	push ds
	push es
	push fs
	push gs
	pushad
	;pushad压入顺序：EAX，ECX，EDX，EBX，ESP，EBP，ESI，EDI
	
	push 0x80	;压入中断号，保持格式统一
	
;第二步：为系统调用子功能传入参数
	push edx	;系统调用中第三个参数
	push ecx	;系统调用中第二个参数
	push ebx	;系统调用中第一个参数
	
;第三步：调用子功能处理函数
	call [syscall_table + eax*4]
	add esp, 12 	;跨过以上三个参数

;第四步：将返回值传入eax
	mov [esp + 8*4], eax	;将返回值赋值给上面保存的eax地址处
	jmp intr_exit			;从中断中返回，恢复上下文
