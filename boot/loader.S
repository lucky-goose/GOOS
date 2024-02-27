%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

;构建gdt以其内部的描述符
	GDT_BASE: dd 0x00000000
				dd 0x00000000	;第一个描述符不可用

	CODE_DESC: dd 0x0000FFFF
				dd DESC_CODE_HIGH4

	DATA_STACK_DESC: dd 0x0000FFFF
					 dd DESC_DATA_HIGH4

	VIDEO_DESC: dd 0x80000007	;limit = (0xbffff-0xb8000)/4k = 0x7
				dd DESC_VIDEO_HIGH4	;此时dpl为0

	GDT_SIZE equ $ - GDT_BASE
	GDT_LIMIT equ GDT_SIZE - 1

	times 60 dq 0		;此处预留60个描述符空位
	
	SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0 	;相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
	SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0		;同上
	SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0	;同上
	
	;total_mem_bytes 用于保存内存容量，以字节为单位
	;当前偏移loader.bin文件头0x200字节
	;loader.bin的加载地址是0x900
	;故total_mem_bytes的地址是0xb00

	total_mem_bytes dd 0
	
	;以下是gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址

	gdt_ptr dw GDT_LIMIT
		dd GDT_BASE
				
	;人工对齐，total_mem_bytes(4) + gdt_ptr(6) + ards_buf(244) + ards_nr(2) = 256 字节
	ards_buf times 244 db 0
	ards_nr dw 0 		;用于记录ARDS结构体数量
	
	loader_start:
	
; int 15h eax = 0000E820h, edx = 534D4150h ('SMAP') 获取内存布局

	xor ebx, ebx			;第一次调用时，ebx要为0
	mov edx, 0x534d4150		;edx只赋值一次，循环体中不会改变
	mov di, ards_buf		;ards结构缓冲区
	
.e820_mem_get_loop:			;循环获取每个ARDS内存范围描述结构
	mov eax, 0x0000e820		;执行int 0x15后， eax会变为0x534d4150
							;所以每次int前都要将eax更新为功能号
	mov ecx, 20				;ARDS地址范围描述符结构大小是20字节
	int 0x15				
	add di, cx				;使di增加20字节指向下一个位置
	inc word [ards_nr]		;记录ARDS的数量
	cmp ebx, 0				;若ebx为0，说明ards全部返回
							;当前已是最后一个
	jnz .e820_mem_get_loop
	
							;在所有ards中，
							;找出base_add_low + length_low 的最大值，即内存的容量
	mov cx, [ards_nr]
							;遍历每一个ARDS结构体，循环次数是ARDS的数量
	mov ebx, ards_buf
	xor edx, edx 			;edx是最大内存的数量，在此先清0
	
.find_max_mem_area:
;无需判断type是否为1，最大的内存块一定是可被使用的
	mov eax, [ebx]			;base_add_low
	add eax, [ebx+8]		;length_low
	add ebx, 20				;指向缓冲区中下一个ARDS结构
	cmp edx, eax
;冒泡排序，找出最大，edx寄存器始终是最大的内存容量
	jge .next_ards
	mov edx, eax
	
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok
	
.mem_get_ok:
	;将内存转为byte后存入total_mem_bytes处
	mov [total_mem_bytes], edx		
				
				
;---准备进入保护模式---
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1

	;第一步:打开A20
	in al,0x92
	or al,0000_0010B
	out 0x92, al

	;第二步:加载GDT
	lgdt [gdt_ptr]
	
	;第三步:cr0第3位置1
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp dword SELECTOR_CODE:p_mode_start	;刷新流水线

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp,LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax

	mov byte [gs:160], 'P'

;------加载kernel------
	mov eax, KERNEL_START_SECTOR ;kernel.bin 所在的扇区号
	mov ebx, KERNEL_BIN_BASE_ADDR ;写入到ebx指定的地址
	
	mov ecx, 200 ;读入的扇区数

	call rd_disk_m_32



;第一步：创建页目录表并初始化页内存位图
call setup_page
	;将全局描述符表的地址读入gdt_ptr
	sgdt [gdt_ptr]
	
	;低2字节是界限，所以+2
	mov ebx, [gdt_ptr + 2]
	;将视频段的地址移到高1GB
	;视频段是第三个段描述符，所以8 * 3 = 0x18
	or dword [ebx + 0x18 + 4], 0xc0000000
	
	;将gdt的基地址移动到高1GB
	add dword [gdt_ptr + 2], 0xc0000000
	;将栈指针也移动到高1GB
	add esp, 0xc0000000
	
;第二步：将页目录表地址赋给cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
	
;第三步：将控制寄存器cr0的pg位(第31位)置1
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	
	;启动分页后，用新的gdt地址重新加载
	lgdt [gdt_ptr]
	
	mov byte [gs:160], 'W'
	
	call kernel_init
	mov esp, 0xc009f000
	jmp KERNEL_ENTRY_POINT

	jmp $

;----创建页目录表及页表 ------
setup_page:
;先将页目录表的4kb初始化为0
	mov ecx, 4096
	mov esi, 0
.create_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .create_page_dir
	
;开始创建页目录项(PDE)
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000			;此时eax为第一个页表的地址
	mov ebx, eax
	or eax, PG_US_U | PG_RW_W | PG_P
	;第一个页目录项和第768个页目录项映射到第一个页表，
	mov [PAGE_DIR_TABLE_POS + 0x0], eax
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax
	
	;使页目录表最后一项指向页目录表自己的地址
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax
	
;下面创建页表项
	mov ecx, 256		;1MB低端内存/4kb = 256页
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
	mov [ebx + esi * 4], edx
	inc esi
	add edx, 4096
	loop .create_pte
	
;创建内核地址空间(1GB）的其他页表的PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000			;此时eax为第二个页表的位置
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov esi, 769			;从第769个页目录项开始
	mov ecx, 254 			;高1GB共256个页目录项，除去第一个和最后一个，共254个
.create_kernel_pde:
	mov [ebx + esi * 4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_pde
	ret

	
;功能：读取硬盘n个扇区 
rd_disk_m_32:		;eax = LBA扇区号 bx = 将数据写入的内存地址 cx = 读入的扇区数
    mov esi,eax		;备份eax
    mov edi,ecx		;备份cx
;读写硬盘
;第一步:设置要读取的扇区数
    mov dx,0x1f2	
    mov al,cl
    out dx,al
   
    mov eax,esi		;恢复eax
    
;第二步:将LBA地址存入0x1f3 ~ 0x1f6
    
    ;LBA地址7~0位写入端口0x1f3
    mov dx,0x1f3
    out dx,al
    
    ;LBA地址15~8位写入端口0x1f4
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al
    
    ;LBA地址23~16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al
    
    shr eax,cl
    and al,0x0f		;LBA第24~27位
    or al,0xe0		;设置7~4位为1110,表示LBA模式
    mov dx,0x1f6
    out dx,al    
    
;第三步：向0x1f7端口写入读命令，0x20
    mov dx,0x1f7
    mov al,0x20
    out dx,al
    
;第四步：检测硬盘状态
.not_ready:
	;同一端口，写时表示写入命令数，读时表示读入硬盘状态
  	nop
  	in al,dx
  	and al,0x88		;第3位为1表示硬盘控制器已准备好数据传输
  					;第7位为1表示硬盘忙
  	cmp al,0x08		
  	jnz .not_ready	;若未准备好，继续等
  	
 ;第五步：从0x1f0端口读数据
  	mov ax,di
  	mov dx,256
  	mul dx
  	mov cx,ax
;di为要读取的扇区数，一个扇区有512字节，每次读入2个字节
;共需di*512/2次，所以di*256
  	mov dx,0x1f0

.go_on_read:
	in ax,dx
	mov [ebx],ax
	add ebx,2
	loop .go_on_read
	ret
	
;-----将kernel.bin中的segment拷贝到编译的地址-------
kernel_init:
	xor eax, eax
	xor ebx, ebx	;ebx记录程序头表的地址
	xor ecx, ecx	;ecx记录程序头表中program header数量
	xor edx, edx	;edx记录program header尺寸,即e_phentsize
	
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]		;e_phentsize
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]	;e_phoff
	add ebx, KERNEL_BIN_BASE_ADDR			
	mov cx, [KERNEL_BIN_BASE_ADDR + 44] 	;e_phnum
	
.each_segment:
	cmp byte [ebx + 0], PT_NULL
;若p_type等于PT_NULL说明改程序头未使用
	je .PTNULL
	
	;为函数memcpy(dst,src,size)压入参数，顺序为从右往左
	;第三个参数:size
	push dword [ebx + 16]	;p_filesz
	
	;第二个参数:src
	mov eax, [ebx + 4]	;p_offset
	add eax, KERNEL_BIN_BASE_ADDR
	push eax
	
	;第三个参数:dst
	push dword [ebx + 8] ;p_vadrr
	
	call mem_cpy
	add esp, 12	;清除栈中的三个参数

.PTNULL:
	add ebx, edx	;令ebx指向下一个program header
	loop .each_segment
	ret
	
;------mem_cpy(dst,src,size)------
;输入：栈中三个参数(dst, src, size)
;输出：无
;---------------------------------
mem_cpy:
	cld
	push ebp
	mov ebp, esp
	push ecx	;rep指令用到了ecx，故先备份
	
	mov edi, [ebp + 8]	;dst
	mov esi, [ebp + 12]	;src
	mov ecx, [ebp + 16]	;size
	rep movsb			;逐字节拷贝
	
	;恢复环境
	pop ecx 
	pop ebp
	ret
