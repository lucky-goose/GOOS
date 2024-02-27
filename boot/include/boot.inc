;---loader和kernel--- 
LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2
PAGE_DIR_TABLE_POS equ 0x100000
KERNEL_START_SECTOR equ 0x9
KERNEL_BIN_BASE_ADDR equ 0x70000
KERNEL_ENTRY_POINT equ 0xc0001500

;---gdt描述符属性---
DESC_G_4K equ 1_00000000000000000000000b  ;第23位，表示4k粒度或者1mb粒度，此处为1表示为4k
DESC_D_32 equ 1_0000000000000000000000b   ;第22位D/B位，对代码段来说是D位，在此表示32位操作数
DESC_L	  equ 0_000000000000000000000b    ;第21位L位，为0表示32位代码段，为1表示64位代码段

DESC_AVL equ 0_00000000000000000000b      ;第20位，供操作系统使用的位，可不设置

DESC_LIMIT_CODE2 equ 1111_0000000000000000b ;代码段的段界限的第二部分，第一部分在段描述符的低4字节中
DESC_LIMIT_DATA2 equ DESC_LIMIT_CODE2	      ;数据段的段界限的第二部分
DESC_LIMIT_VIDEO2 equ 0000_0000000000000000b ;显存区描述符

DESC_P equ 1_000000000000000b			;第15位P(present)位，1表示该段在内存中
DESC_DPL_0 equ 00_0000000000000b		;该段描述符对应的内存段的特权级别，0最高，3最低
DESC_DPL_1 equ 01_0000000000000b
DESC_DPL_2 equ 10_0000000000000b
DESC_DPL_3 equ 11_0000000000000b

DESC_S_CODE equ 1_000000000000b		;代码段的S位，为1则表示是普通的内存段，不是系统段
DESC_S_DATA equ DESC_S_CODE			;
DESC_S_sys equ 0_000000000000b

;x=1,c=0,r=0,a=0代码段是可执行的，非一致性，不可读，已访问位a清0
DESC_TYPE_CODE equ 1000_00000000b

;x=0,e=0,w=1,a=0数据段是不可执行的，向上扩展的，可读，已访问位清0
DESC_TYPE_DATA equ 0010_00000000b

;代码段描述符的高四字节
DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
DESC_P + DESC_DPL_0 + DESC_S_CODE +\
DESC_TYPE_CODE + 0x00

;数据段描述符的高四字节
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + \
DESC_TYPE_DATA + 0x00

;显存段描述符的高四字节
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + \
DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0B

;---选择子属性---
;选择子第0-2位为PRL，表示请求特权级，第3位为0表示在GDT中索引，为1表示在LDT中索引
RPL0 equ 00b
RPL1 equ 01b
RPL2 equ 10b
RPL3 equ 11b
TI_GDT equ 000b
TI_LDT equ 100b

;----页表相关属性-----
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

;-----elf头文件相关属性---
PT_NULL equ 0

