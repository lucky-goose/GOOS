[bits 32]
section .text 
global switch_to
switch_to:
	;栈中此处是函数调用时压入的返回地址
	push esi
	push edi
	push ebx
	push ebp
	
	
	;得到cur的地址
	mov eax, [esp + 20]
	;保存cur的栈指针
	;pcb的偏移0处是self_kstack
	mov [eax], esp
	
	;以上是保存cur的环境，接下来恢复next的环境
	
	;得到next的地址
	mov eax, [esp + 24]
	;恢复栈指针
	mov esp, [eax]
	
	;恢复寄存器
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
