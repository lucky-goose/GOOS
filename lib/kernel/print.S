TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

[bits 32]
section .text
;--------- put_char------
;功能描述：把栈中的一个字符写入光标所在处
;------------------------
global put_char
put_char:
	;保存所有双字寄存器
	pushad
	;保证gs寄存器中是视频段的选择子
	mov ax, SELECTOR_VIDEO
	mov gs, ax

	;获取光标地址
	;获取光标地址的高8位	
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	in al, dx
	mov ah, al

	;获取光标地址的低8位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx

	;将光标地址保存到ebx
	mov bx, ax
	;获取待打印的字符
	mov ecx, [esp + 36]
	
	;CR是0x0d，LF是0x0a
	cmp cl, 0x0d
	je .is_carriage_return 

	cmp cl, 0x0a
	je .is_line_feed

	;BackSpace 的asc码是0x08
	cmp cl, 0x08
	je .is_backspace
	jmp .put_other

.is_backspace:
;------backspace操作：先将光标往前移动一位，再将该位置改为空格------
	dec bx
	shl bx, 1
	mov word [gs:bx], 0x0720
	shr bx, 1
	jmp .set_cursor	

.put_other:
	;bx*2 等于偏移的字节数
	;写入字符和属性
	shl bx, 1
	mov byte [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	;恢复光标并指向下一个位置
	shr bx, 1
	inc bx
	;如果超出页面则卷屏
	cmp bx, 2000
	jl .set_cursor	
	jmp .roll_screen

.is_line_feed:
.is_carriage_return:
	xor dx, dx	;dx是被除数的高16位，清0
	mov ax, bx	;ax是被除数的低16位
	mov si, 80	;将除数存入si

	div si

	sub bx, dx	;用bx减去余数，令bx指向当前行的首端

.is_carriage_return_end:
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:
	jl .set_cursor
	jmp .roll_screen

;屏幕范围是0~24行
;滚屏将1～24行移置0~23行，并将第24行清0
.roll_screen:
	cld 
	;共1920个字符，3840字节
	;一次搬运4字节，共需搬运960次
	mov ecx, 960
	
	mov esi, 0xc00b80a0 	;第1行行首
	mov edi, 0xc00b8000	;第0行行首
	rep movsd
	
;----将最后一行清0----

	mov ebx, 3840	;最后一行首字符偏移：1920*2 = 3840
	mov ecx, 80		;最后一行共80个字符

.cls:
	mov word [gs:bx], 0x0720
	add bx, 2
	loop .cls
	;将光标移到最后一行行首
	mov bx, 1920

.set_cursor:
;将光标设为bx的值
;-----先设置bx的高8位---
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al

;-----再设置bx的低8位---
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al

.put_char_done:
	popad
	ret

[bits 32]
section .text
;----------------------------------
;---put_str 打印以0为结尾的字符串--
;----------------------------------
;输入:栈中参数为打印的字符串
;输出：无
global put_str
put_str:
	push ebx
	push ecx
	xor ecx, ecx		;用ecx来存储字符
	mov ebx, [esp + 12] ;用ebx存储当前处理的地址
	
.go_on:
	;如果当前字符为0，则跳出循环
	mov cl, [ebx]	
	cmp cl, 0
	je .str_over
	;参数入栈，调用put_char
	push ecx
	call put_char
	;回收栈空间，指向下一个字符
	add esp, 4
	inc ebx
	jmp .go_on

.str_over:
	pop ecx
	pop ebx
	ret

section .data
put_int_buffer dq 0	;定义8字节的缓冲区用于字符转换

;------打印32位整数------
;输入：栈中为待打印的整数
;输出：打印数字的16进制格式，没有前置的0x

section .text
global put_int
put_int:
	pushad
	mov ebp, esp
	mov eax, [ebp + 4 * 9]
	mov edx, eax			;将要转换的数字
	mov edi, 7				;buffer中的索引
	mov ecx, 8				;共有8个16进制位
	mov ebx, put_int_buffer
	
.16based_4bits:
	and edx, 0x0000000F		;获取低4位
	cmp edx, 9
	jg .is_A2F				;如果是0-9则+‘0’
	add edx, '0'
	jmp .store

.is_A2F:					;如果是a-f则+‘A’-10
	sub edx, 10
	add edx, 'A'

.store:
	;将当前的字符存储在缓存中
	;并处理下一个字符
	mov [ebx + edi], dl
	dec edi
	shr eax, 4
	mov edx, eax
	loop .16based_4bits

.ready_to_print:
	;令edi为0
	inc edi
.skip_prefix_0:
	;若当前检测到了第8个字符，说明全为0
	cmp edi, 8
	je .full_0

.go_on_skip:
	;比较字符并指向下一个字符
	mov cl, [ebx + edi]
	inc edi
	cmp cl, '0'
	je .skip_prefix_0
	;指向第一个不为0的字符，并跳转
	dec edi
	jmp .put_each_num

.full_0:
	mov cl, '0'
.put_each_num:
	;利用put_char函数依次打印所有的字符
	push ecx
	call put_char
	add esp, 4
	inc edi
	mov cl, [ebx + edi]
	cmp edi, 8
	jl .put_each_num

	popad
	ret
	
	
[bits 32]
section .text
;----------------------------------
;---set_cursor 设置光标位置--
;----------------------------------
;输入:光标位置（字节）
;输出：无
global set_cursor
set_cursor:
	push eax
	push ebx
	push edx
	mov ebx, [esp + 16]
	;将光标设为bx的值
;-----先设置bx的高8位---
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al

;-----再设置bx的低8位---
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al
	
	pop edx
	pop ebx
	pop eax
	ret
