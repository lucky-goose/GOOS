%include "boot.inc"
SECTION MBR vstart=0x7c00 ;起始地址编译在0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax
    ;这个时候 ds = es = ss = 0 栈指针指向MBR开始位置

;清屏
;利用0x06号功能,上卷全部行，即可清屏
;INT 0x10 功能号:0x06 功能描述：上卷窗口
;输入：
;AH 功能号 = 0x06
;AL = 上卷行数 （如果为0，表示全部）
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(x,y)位置
;(DL,DH) = 窗口右下角的(x,y)位置
;无返回值
	mov ax,0600h
	mov bx,0700h
	mov cx,0
	mov dx,184fh
;因为VGA文本模式中，一行只能容纳80个字符，共25行
;下标从0开始，所以0x18 = 24, 0x4f = 79
	int 0x10
    		
    mov eax,LOADER_START_SECTOR 	;起始扇区lba地址
    mov bx,LOADER_BASE_ADDR			;写入的地址
    mov cx,4						;待读入的扇区数
    call rd_disk_m_16				;以下读取程序的起始部分
    
    jmp LOADER_BASE_ADDR + 0x300

;功能：读取硬盘n个扇区 
rd_disk_m_16:		;eax = LBA扇区号 bx = 将数据写入的内存地址 cx = 读入的扇区数
    mov esi,eax		;备份eax
    mov di,cx		;备份cx
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
;di为要读取的扇区数，一个扇区有512字节，每次读入一个字
;共需di*512/2次，所以di*512 
  	mov dx,0x1f0

.go_on_read:
	in ax,dx
	mov [bx],ax
	add bx,2
	loop .go_on_read
	ret
	
  	
    ;字符串声明 db == define byte dw == define word ascii一个字符占一个字节
    
    
    ;预留两个字节 其余空余的全部用0填满 为使检测当前扇区最后两字节为0x55 0xaa 检测是否为有效扇区
    ;510 = 512字节-2预留字节  再减去（当前位置偏移量-段开始位置偏移量）求出来的是剩余空间
    times 510 - ($ - $$) db 0 
    db 0x55,0xaa

