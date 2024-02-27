#include "process.h"
#include "global.h"
#include "tss.h"
#include "debug.h"
#include "string.h"
#include "console.h"
#include "memory.h"
#include "interrupt.h"
#include "list.h"

extern void intr_exit(void);

//构建用户进程初始上下文信息
void start_process(void* filename_) {
	void* function = filename_;

	//获取当前进程的栈指针
	struct task_struct* cur = running_thread();
	cur->self_kstack += sizeof(struct thread_stack);
	struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
	
	
	//初始化中段栈
	proc_stack->edi = 0;
	proc_stack->esi = 0;
	proc_stack->ebp = 0;
	proc_stack->esp_dummy = 0;
	proc_stack->ebx = 0;
	proc_stack->edx = 0;
	proc_stack->ecx = 0;
	proc_stack->eax = 0;
	//用户态不能操作显存，直接初始化为0
	proc_stack->gs = 0;
	proc_stack->ds = SELECTOR_U_DATA;
	proc_stack->es = SELECTOR_U_DATA;
	proc_stack->fs = SELECTOR_U_DATA;
	proc_stack->eip = function;
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
	proc_stack->ss = SELECTOR_U_DATA;
	
	
	asm volatile ("movl %0 , %%esp ; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

//激活页表
void page_dir_activate(struct task_struct* pthread) {
	//默认更换为内核的页目录表
	uint32_t pagedir_phy_addr = 0x100000;
	//如果下一个线程有自己的页表，就更换为他的页表
	if (pthread->pgdir != NULL) {
		pagedir_phy_addr = addr_v2p((uint32_t)pthread->pgdir);
	}
	//更新页目录表寄存器cr3，刷新页表
	asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

//激活线程或进程的页表，更新tss中esp0为进程的特权级0的栈
void process_activate(struct task_struct* pthread) {
	ASSERT(pthread != NULL);
	//激活页表
	page_dir_activate(pthread);
	//如果是用户进程，则在tss中写入0级栈的地址
	if (pthread->pgdir) {
		update_tss_esp(pthread);
	}
}

//创建页目录表，将当前页表的表示内核的pde复制，成功则返回页表的虚拟地址，否则返回-1
uint32_t* create_page_dir(void) {
	//用户进程的页表只能由内核维护，所以在内核空间申请
	uint32_t* page_dir_vaddr = get_kernel_pages(1);
	if (page_dir_vaddr == NULL) {
		console_put_str("create_page_dir: get_kernel_page failed!");
		return NULL;
	}
	
	//先复制页表
	//把内核目录表的768项到1023项复制到用户页表中，共享内核
	//0x300是768，一个页目录项占4字节
	//通过虚拟地址0xfffff000访问到PDT的地址
	memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);
	
	//再更新页目录地址
	//将页目录表最后一项更新为页目录表自己的物理地址
	uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
	page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
	return page_dir_vaddr;
}

//创建用户进程虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
	user_prog->userprog_vaddr_pool.vaddr_start = USER_VADDR_START;
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
	user_prog->userprog_vaddr_pool.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
	user_prog->userprog_vaddr_pool.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
	bitmap_init(&user_prog->userprog_vaddr_pool.vaddr_bitmap);
}

//创建用户进程
void process_execute(void* filename, char* name) {
	//在内核中申请一页用作pcb
	struct task_struct* thread = get_kernel_pages(1);
	
	//初始化进程pcb
	init_thread(thread, name, DEFAULT_PRIO);
	//创建进程虚拟地址池
	create_user_vaddr_bitmap(thread);
	//创建并初始化线程栈
	thread_create(thread, start_process, filename);
	//创建页表
	thread->pgdir = create_page_dir();
	//初始化内存块描述符
	block_desc_init(thread->u_block_descs);
	//将新进程加入队列
	enum intr_status old_status = intr_disable();
	ASSERT(!node_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);
	
	ASSERT(!node_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);
	
	set_intr_status(old_status);
}

