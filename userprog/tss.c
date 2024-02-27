#include "tss.h"
#include "global.h"
#include "print.h"
#include "string.h"

//更新tss中的esp0字段的值为pthread的0级栈
void update_tss_esp(struct task_struct* pthread) {
	tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

//创建gdt描述符
struct gdt_desc make_gdt_desc(uint32_t* desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high) {
	uint32_t desc_base = (uint32_t)desc_addr;
	struct gdt_desc desc;
	desc.limit_low_word = limit & 0x0000ffff;
	desc.base_low_word = desc_base & 0x0000ffff;
	desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
	desc.attr_low_byte = (uint8_t)(attr_low);
	desc.limit_high_attr_high = ((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high);
	desc.base_high_byte = desc_base >> 24;
	return desc;
}

//在gdt中创建tss并重新加载gdt
void tss_init(void) {
	put_str("tss_init start\n");
	
	uint32_t tss_size = sizeof(tss);
	memset(&tss, 0, tss_size);
	tss.ss0 = SELECTOR_K_STACK;
	//没有io位图
	tss.io_base = tss_size;
	
	//在gdt中添加dpl为0的TSS描述符
	*((struct gdt_desc*)0xc0000920) = make_gdt_desc((uint32_t*)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
	
	//在gdt中添加dpl为3的数据段和代码段描述符
	*((struct gdt_desc*)0xc0000928) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
	
	*((struct gdt_desc*)0xc0000930) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
	
	uint64_t gdt_ptr = (8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16);
	
	//重新载入gdt指针
	asm volatile ("lgdt %0" : : "m" (gdt_ptr));
	//载入tr寄存器
	asm volatile ("ltr %w0" : : "r" (SELECTOR_TSS));
	
	put_str("tss_init done\n");
}
