#include "interrupt.h"
#include "global.h"
#include "print.h"
#include "io.h"

#define IDT_DESC_CNT 0x81		//目前支持0x81个中断
#define EFLAGS_IF 0x00000200	//eflags寄存器中的IF位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))

//IDT中断描述符表
static struct gate_desc idt[IDT_DESC_CNT];

//中断异常名
char* intr_name[IDT_DESC_CNT];

//中断处理程序数组
intr_handler handlers[IDT_DESC_CNT];

//系统调用中断处理程序
extern uint32_t syscall_handler(void);

//定义在kernel.S中的中断处理程序入口数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];

//初始化中断描述符
void init_idt_desc(struct gate_desc* g_desc, uint8_t attr, intr_handler function){
	g_desc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
	g_desc->selector = SELECTOR_K_CODE;
	g_desc->dcount = 0;
	g_desc->attribute = attr;
	g_desc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

//初始化中断描述符表
void init_idt() {
	int i,lastindex = IDT_DESC_CNT - 1;
	for (i = 0; i < IDT_DESC_CNT; i++) {
		init_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	//系统调用的中断描述符单独处理
	//描述符特权级别、为3，才能从用户态直接
	init_idt_desc(&idt[lastindex], IDT_DESC_ATTR_DPL3, syscall_handler);
	
	put_str("idt _desc_init done\n");	
}

//初始化可编程中断控制器8259A
void init_pic() {
	//初始化主片
	outb(PIC_M_CTRL, 0x11);		//ICW1:边沿触发，级联8259，需要ICW4
	outb(PIC_M_DATA, 0x20);		//ICW2:起始中断向量号为0x20
	
	outb(PIC_M_DATA, 0x04);		//ICW3:IR2接从片
	outb(PIC_M_DATA, 0x01);		//ICW4:8086模式，正常EOI
	
	//初始化从片
	outb(PIC_S_CTRL, 0X11);		//ICW1:边沿触发，级联8259，需要ICW4
	outb(PIC_S_DATA, 0x28);		//ICW2:起始中断向量号为0x28
	
	outb(PIC_S_DATA, 0x02);		//ICW3:设置从片连接到主片的IR2引脚
	outb(PIC_S_DATA, 0x01);		//ICW4:8086模式， 正常EOI
	
	//打开主片上的IR0，也就是目前只接受时钟产中断和键盘中断
	outb(PIC_M_DATA, 0xfc);
	outb(PIC_S_DATA, 0xFF);
	
	put_str("init pic done\n");
}

//默认中断处理程序
void general_intr_handler(uint8_t vec_index) {
	if (vec_index == 0x27 || vec_index == 0x2f) {
		//IRQ7和IRQ15会产生伪中断，故直接返回
		return;
	}
	
	//将光标置0，从屏幕左上角清出一片区域打印异常信息
	set_cursor(0);
	uint32_t cursor_pos = 0;
	while (cursor_pos < 320) {
		put_char(' ');
		cursor_pos++;
	}
	
	set_cursor(0);
	put_str("!!! exception message begin !!!\n");
	//从第二行第八个字符开始打印
	set_cursor(88);
	put_str(intr_name[vec_index]);
	
	//若为pagefault，将缺失的地址打印出来并悬停
	if (vec_index == 14) {
		uint32_t page_fault_vaddr = 0;
		asm volatile ("movl %%cr2, %0" : "=r" (page_fault_vaddr));
		
		put_str("\npage fault addr is 0x");
		put_int(page_fault_vaddr);
	}
	put_str("\n!!! exception message end !!!\n");
	//因为进入中断时已经关闭中断，所以这里会无限循环
	while(1);
}

//完成默认中断处理程序注册和异常名称注册
void init_exception() {
	int i;
	for (i = 0; i < IDT_DESC_CNT; i++) {
		handlers[i] = general_intr_handler;
		intr_name[i] = "unknown";
	}	
	intr_name[0] = "#DE Divide Error";
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR BOUND Range Exceeded Exception";
	intr_name[6] = "#UD Invalid Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invalid TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	//intr_name[15] 第15项是intel保留项，未使用
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
	intr_name[32] = "timer interrupt";
}

//注册中断处理函数
void register_handler(uint8_t intr_num, intr_handler function) {
	handlers[intr_num] = function;
}

//初始化中断的相关工作
void init_intr() {
	put_str("interrupt start\n");
	init_idt();			//初始化中断描述符表
	init_exception(); 	//初始化异常名称和注册默认中断处理程序
	init_pic();			//初始化8259A

	//加载idt
	uint64_t idt_ptr = ((uint64_t)(uint32_t)idt << 16) | (sizeof(idt) - 1);
	asm volatile ("lidt %0" : : "m" (idt_ptr));
	put_str("interrupt init done\n");
}

//开中断并返回之前的状态
enum intr_status intr_enable(void){
	enum intr_status old_status;
	old_status = get_intr_status();
	if (old_status == INTR_ON) {
		return old_status;
	} else {
		asm volatile ("sti");
		return old_status;
	}
}

//关中断并返回之前的状态
enum intr_status intr_disable(void){
	enum intr_status old_status;
	old_status = get_intr_status();
	if (old_status == INTR_ON){
		asm volatile ("cli" : : :"memory");
		return old_status;
	} else {
		return old_status;
	} 
}

//设置中断状态并返回之前的状态
enum intr_status set_intr_status(enum intr_status status){
	return (status & INTR_ON) ? intr_enable() : intr_disable(); 
}

//获取中断状态
enum intr_status get_intr_status(void){
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}
