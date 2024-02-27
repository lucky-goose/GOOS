#include "memory.h"
#include "thread.h"
#include "global.h"
#include "print.h"
#include "string.h"
#include "debug.h"
#include "interrupt.h"

//初始化内存池
void mem_pool_init(uint32_t all_mem) {
	put_str("mem_pool_init start\n");
	
	//初始化锁
	lock_init(&kernel_mem_pool.lock);
	lock_init(&user_mem_pool.lock);
	
	//已创建的页表：1页的页目录表+第0和第768个页目录项指向的第一个页表 + 
	//第769 - 1022个页目录项共指向254个页表，共256个页表
	uint32_t page_table_size = PG_SIZE * 256;
	
	//已用的内存为低端1MB+页表占用的内存
	uint32_t used_mem = page_table_size + 0x100000;
	
	//空余的内存和页数
	uint32_t free_mem = all_mem - used_mem;
	uint32_t free_pages = free_mem / PG_SIZE;
	
	//内核和用户空余页数
	uint32_t kernel_free_pages = free_pages / 2;
	uint32_t user_free_pages = free_pages - kernel_free_pages;
	
	//初始化内存池的物理起始地址
	kernel_mem_pool.phy_addr_start = used_mem;
	user_mem_pool.phy_addr_start = used_mem + kernel_free_pages * PG_SIZE;
	
	//初始化内存池的内存大小（字节）
	kernel_mem_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_mem_pool.pool_size = user_free_pages * PG_SIZE;
	
	//内存池bitmap的大小
	uint32_t kbm_len = kernel_free_pages / 8;
	uint32_t ubm_len = user_free_pages / 8;
	
	
	//为简化操作，余数不作处理，坏处是会丢失内存
	//好处是不用做内存的越界检查
	//初始化bitmap的大小
	kernel_mem_pool.mem_pool_bitmap.btmp_bytes_len = kbm_len;
	user_mem_pool.mem_pool_bitmap.btmp_bytes_len = ubm_len;
	
	//初始化bitmap数组的地址
	kernel_mem_pool.mem_pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_mem_pool.mem_pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_len);
	
	//将位图置0
	bitmap_init(&kernel_mem_pool.mem_pool_bitmap);
	bitmap_init(&user_mem_pool.mem_pool_bitmap);
	
	//输出内存池信息
	put_str("kernel_mem_pool_bitmap_start:");
	put_int((uint32_t)kernel_mem_pool.mem_pool_bitmap.bits);
	put_char('\n');
	put_str("kernel_mem_pool_phy_addr_start:");
	put_int(kernel_mem_pool.phy_addr_start);
	put_char('\n');
	
	put_str("user_mem_pool_bitmap_start:");
	put_int((uint32_t)user_mem_pool.mem_pool_bitmap.bits);
	put_char('\n');
	put_str("user_mem_pool_phy_addr_start:");
	put_int(user_mem_pool.phy_addr_start);
	put_char('\n');
	
	//下面初始化内核虚拟地址池
	kernel_vaddr_pool.vaddr_bitmap.btmp_bytes_len = kbm_len;
	kernel_vaddr_pool.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_len + ubm_len);
	bitmap_init(&kernel_vaddr_pool.vaddr_bitmap);
	
	kernel_vaddr_pool.vaddr_start = K_HEAP_START;
	
	
	put_str("mem_pool_init done\n");
}

//初始化内存管理系统
void mem_init(void) {
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
	//初始化内存池
	mem_pool_init(mem_bytes_total);
	//初始化内存块描述符，为malloc做准备
	block_desc_init(k_block_descs);
	put_str("mem_init done\n");	
}

//在虚拟内存池中申请pg_cnt个虚拟页，成功则返回起始地址，失败则返回NULL
void* get_virtual_addr(enum pool_flags pf, uint32_t pg_cnt) {
	uint32_t vaddr_start = 0, cnt = 0;
	int32_t bit_idx_start = -1;
	if (pf == PF_KERNEL) {
		//在内存池中找到连续cnt个空闲位
		bit_idx_start = bitmap_scan(&kernel_vaddr_pool.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1) return NULL;
		//设置空闲位为1，并返回物理地址
		while (cnt < pg_cnt) {
			bitmap_set(&(kernel_vaddr_pool.vaddr_bitmap), bit_idx_start + cnt, 1);
			cnt++;
		}
		vaddr_start = kernel_vaddr_pool.vaddr_start  + bit_idx_start * PG_SIZE;
	} else {
		//用户池内存池
		//在内存池中找到连续cnt个空闲位
		struct task_struct* cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr_pool.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1) return NULL;
		//设置空闲位为1，并返回物理地址
		while (cnt < pg_cnt) {
			bitmap_set(&cur->userprog_vaddr_pool.vaddr_bitmap, bit_idx_start + cnt, 1);
			cnt++;
		}
		vaddr_start = cur->userprog_vaddr_pool.vaddr_start + bit_idx_start * PG_SIZE;
		//0xc0000000 - PG_SIZE处的页作为用户3级栈已被分配
		ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
	}
	return (void*)vaddr_start;
}

//得到虚拟地址对应的pte指针
uint32_t* pte_ptr(uint32_t vaddr){
	uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}

//得到虚拟地址对应的pde指针
uint32_t* pde_ptr(uint32_t vaddr) {
	uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
	return pde;
}

//在m_pool指向的内存池中分配1个物理页，成功则返回物理地址，失败则返回NULL
void* palloc(struct mem_pool* m_pool) {
	//找到一个物理页
	int bit_idx = bitmap_scan(&(m_pool->mem_pool_bitmap), 1);
	if (bit_idx == -1) return NULL;
	//将位图的该位设置位1，并返回物理地址
	bitmap_set(&(m_pool->mem_pool_bitmap), bit_idx, 1);
	uint32_t page_phyaddr = m_pool->phy_addr_start + bit_idx * PG_SIZE;
	return (void*)page_phyaddr;
}

//在页表中添加_vaddr到_paddr的映射
void page_table_map(void* _vaddr, void* _paddr) {
	uint32_t vaddr = (uint32_t)_vaddr, paddr = (uint32_t)_paddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);
	
	//需要先判断页表存不存在
	if (*pde & 0x00000001) {
		//判断页目录项是否存在
		ASSERT(!(*pte & 0x00000001));
		
		if (!(*pte & 0x00000001)) {
			*pte = (paddr | PG_US_U | PG_RW_W | PG_P_1);	
		} else {
			PANIC("pte repeat");
			*pte = (paddr | PG_US_U | PG_RW_W | PG_P_1);
		}
	} else {
		//先创建页表,页表所用的页框一律从内核空间分配
		uint32_t pgt_paddr = (uint32_t)palloc(&kernel_mem_pool);
		
		*pde = (pgt_paddr | PG_US_U | PG_RW_W | PG_P_1);
		//将页表清0
		memset((void*)((uint32_t)pte & 0xfffff000), 0, PG_SIZE);
		
		ASSERT(!(*pte & 0x00000001));
		*pte = (paddr | PG_US_U | PG_RW_W | PG_P_1);	
	}	
}

//分配pg_cnt个页空间，成功则返回起始虚拟地址，失败则返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);
	//malloc_page的原理是三个动作的合成
	//1.通过get_virtual_addr在虚拟内存池中申请虚拟地址
	//2.通过palloc在物理内存池中申请物理页
	//3.通过page_table_map将虚拟地址映射到物理地址
	
	//第一步：获取虚拟地址
	void* vaddr_start = get_virtual_addr(pf, pg_cnt);
	if (vaddr_start == NULL) return NULL;
	
	
	uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
	struct mem_pool* mem_pool = pf &  PF_KERNEL ? &kernel_mem_pool : &user_mem_pool;
	
	//第二步+第三步：获取物理地址并映射
	//由于虚拟地址是连续的，物理地址是不连续的，所以逐个做映射
	for (;cnt > 0; cnt--) {
		void* page_phyaddr = palloc(mem_pool);
		if (page_phyaddr == NULL) return NULL;
		page_table_map((void*)vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}	

//从内核物理内存池中申请pg_cnt页内存，成功则返回其虚拟地址，失败则返回NULL
void* get_kernel_pages(uint32_t pg_cnt) {
	lock_acquire(&kernel_mem_pool.lock);
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL) {
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	lock_release(&kernel_mem_pool.lock);
	return vaddr;
}

//在用户内存空间中申请4k内存，并返回其虚拟地址
void* get_user_pages(uint32_t pg_cnt) {
	lock_acquire(&user_mem_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	if (vaddr != NULL) {
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	lock_release(&user_mem_pool.lock);
	return vaddr;
}

//分配一页内存，并将其和指定的虚拟地址关联
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
	
	
	struct mem_pool* mem_pool = (pf & PF_KERNEL) ? &kernel_mem_pool : &user_mem_pool;
	lock_acquire(&mem_pool->lock);
	
	struct task_struct* cur = running_thread();
	int32_t bit_idx = -1;
	
	//先将虚拟地址对应的位置为1
	if (cur->pgdir != NULL && pf == PF_USER) {
		bit_idx = (vaddr - cur->userprog_vaddr_pool.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx >= 0);
		bitmap_set(&cur->userprog_vaddr_pool.vaddr_bitmap, bit_idx, 1);
	} else if (cur->pgdir == NULL && pf == PF_KERNEL) {
		bit_idx = (vaddr - kernel_vaddr_pool.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx >= 0);
		bitmap_set(&kernel_vaddr_pool.vaddr_bitmap, bit_idx, 1);
	} else {
		PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
	}
	
	
	//获取一个物理页
	void* page_phyaddr = palloc(mem_pool);
	if (page_phyaddr == NULL) return NULL;
	
	//添加映射
	page_table_map((void*)vaddr, page_phyaddr);
	
	lock_release(&mem_pool->lock);
	return (void*)vaddr;
}

//通过虚拟地址得到其物理地址
uint32_t addr_v2p(uint32_t vaddr) {
	//pte中物理页的地址+偏移量即是物理地址
	uint32_t* pte = pte_ptr(vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

//为malloc做准备
void block_desc_init(struct mem_block_desc* desc_array) {
	uint16_t desc_idx, block_size = 16;
	//逐个初始化内存块描述符
	for (desc_idx = 0; desc_idx < MEM_BLOCK_DESC_CNT; desc_idx++) {
		desc_array[desc_idx].block_size = block_size;
		desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
		list_init(&desc_array[desc_idx].free_list);
		block_size *= 2;
	}
}

//返回arena中第idx个内存块的地址
struct mem_block* arena2block(struct arena* a, uint32_t idx) {
	return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

//返回内存块b所在的arena的地址
struct arena* block2arena(struct mem_block* b) {
	return (struct arena*)((uint32_t)b & 0xfffff000);
}

//在堆中申请size字节内存
void* sys_malloc(uint32_t size) {
	enum pool_flags PF;
	struct mem_pool* mem_pool;
	uint32_t pool_size;
	struct mem_block_desc* descs;
	struct task_struct* cur_thread = running_thread();
	
	//判断用哪个内存池
	if (cur_thread->pgdir == NULL) {
		//若为内核线程
		PF = PF_KERNEL;
		pool_size = kernel_mem_pool.pool_size;
		mem_pool = &kernel_mem_pool;
		descs = k_block_descs;
	} else {
		//若为用户进程
		PF = PF_USER;
		pool_size = user_mem_pool.pool_size;
		mem_pool = &user_mem_pool;
		descs = cur_thread->u_block_descs;
	}
	
	//若申请的内存超出容量则返回NULL
	if (size < 0 || size > pool_size) return NULL;
	
	struct arena* a;
	struct mem_block* b;
	
	lock_acquire(&mem_pool->lock);
	
	//如果超过最大内存块1024B，就分配整个页框
	if (size > 1024) {
		//需要分配的页框数
		uint32_t pg_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
		
		//获取页框		
		a = malloc_page(PF, pg_cnt);
		
		//如果分配成功
		if (a != NULL) {
			//初始化清0
			memset(a, 0, pg_cnt * PG_SIZE);
			//初始化arena属性
			a->desc = NULL;
			a->cnt = pg_cnt;
			a->large = true;
			lock_release(&mem_pool->lock);
			//跨国arena的大小，返回实际内存的起始地址
			return (void*)(a + 1);
		} else {
			lock_release(&mem_pool->lock);
			return NULL;
		}
		
	} else {
		uint8_t desc_idx;
		//找到符合大小的内存块规格
		for (desc_idx = 0; desc_idx < MEM_BLOCK_DESC_CNT; desc_idx++) {
			if (size <= descs[desc_idx].block_size) break;
		}
		
		//若mem_block_desc的free_list为空，则创建新的arena
		if (list_empty(&descs[desc_idx].free_list)) {
			a = malloc_page(PF, 1);
			
			if (a == NULL) {
				lock_release(&mem_pool->lock);
				return NULL;
			}
			
			memset(a, 0, PG_SIZE);
				
			//初始化arena的属性
			a->desc = &descs[desc_idx];
			a->cnt = descs[desc_idx].blocks_per_arena;
			a->large = false;	
			
			//接下来将该arena的块都添加进内存块描述符的free_list中
			uint32_t block_idx;
			
			enum intr_status old_status = intr_disable();
			
			for (block_idx = 0; block_idx < a->cnt; block_idx++) {
				b = arena2block(a, block_idx);
				ASSERT(!node_find(&a->desc->free_list, &b->free_block));
				list_append(&a->desc->free_list, &b->free_block);
			}
			
			set_intr_status(old_status);
		}
		
		//开始分配内存块
		b = elem2entry(struct mem_block, free_block, list_pop(&(descs[desc_idx].free_list)));
		
		a = block2arena(b);
		a->cnt--;
		lock_release(&mem_pool->lock);
		return (void*)b;
	}
}

//将物理地址pg_phy_addr回收到物理内存池
void pfree(uint32_t pg_phy_addr) {
	struct mem_pool* mem_pool;
	uint32_t bit_idx = 0;
	
	//判断该内存属于哪个内存池
	if (pg_phy_addr >= user_mem_pool.phy_addr_start) {
		mem_pool = &user_mem_pool;	
	} else {
		mem_pool = &kernel_mem_pool;
	}
	//将该页对应的位图置为0
	bit_idx = (pg_phy_addr - mem_pool->phy_addr_start) / PG_SIZE;
	bitmap_set(&mem_pool->mem_pool_bitmap, bit_idx, 0);
}

//去掉页表中虚拟地址vaddr的映射，只去掉addr对应的pte
void page_table_pte_remove(uint32_t vaddr) {
	//更新pte
	uint32_t* pte = pte_ptr(vaddr);
	*pte &= ~PG_P_1;
	//更新TLB
	asm volatile ("invlpg %0": : "m" (vaddr): "memory");
}

//在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟地址
void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
	uint32_t bit_idx = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
	struct virtual_addr_pool* vaddr_pool;
	
	//判断属于哪个内存池
	if (pf == PF_KERNEL) {
		vaddr_pool = &kernel_vaddr_pool;	
	} else {
		vaddr_pool = &(running_thread()->userprog_vaddr_pool);
	}
	
	//修改位图
	bit_idx = (vaddr - vaddr_pool->vaddr_start) / PG_SIZE;
	
	for (;cnt < pg_cnt; cnt++) {
		bitmap_set(&vaddr_pool->vaddr_bitmap, bit_idx + cnt, 0);
	}
}

//释放以虚拟地址addr起始的cnt个物理页
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
	uint32_t pg_phy_addr = addr_v2p(_vaddr);
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t page_cnt = pg_cnt;
	ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
	
	for (;page_cnt > 0; page_cnt--) {
		//获得物理地址
		pg_phy_addr = addr_v2p(vaddr);
		//将对应的物理页框归还到内存池
		pfree(pg_phy_addr);
		//在页表中取消映射
		page_table_pte_remove(vaddr);
		//处理下一个虚拟页
		vaddr += PG_SIZE;
	}
	//清空虚拟地址在位图中的相应位
	vaddr_remove(pf, _vaddr, pg_cnt);
}

//回收内存ptr
void sys_free(void* ptr) {
	ASSERT(ptr != NULL);
	
	if (ptr == NULL) return;
	
	enum pool_flags PF;
	struct mem_pool* mem_pool;
		
	//判断是内核线程还是用户进程
	if (running_thread()->pgdir == NULL) {
		ASSERT((uint32_t)ptr >= K_HEAP_START);
		PF = PF_KERNEL;
		mem_pool = &kernel_mem_pool;
	} else {
		PF = PF_USER;
		mem_pool = &user_mem_pool;
	}
	
	lock_acquire(&mem_pool->lock);
	//通过ptr得到arena，获取元信息
	struct mem_block* b = ptr;
	struct arena* a = block2arena(b);
	
	//判断是大内存块还是小内存块
	if (a->desc == NULL && a->large == true) {
		//如果是大内存块，直接释放整个页
		mfree_page(PF, a, a->cnt);
	} else {
		//如果是小内存块，先将内存块放会free_list
		list_append(&a->desc->free_list, &b->free_block);
		a->cnt++;
		
		ASSERT(a->large == 0 || a->large == 1);
		//如果arena满，则将整个仓库释放
		if (a->cnt == a->desc->blocks_per_arena) {
			uint32_t block_idx;
			for (block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
				struct mem_block* b = arena2block(a, block_idx);
				ASSERT(node_find(&a->desc->free_list, &b->free_block));
				list_remove(&b->free_block);
			}	
			mfree_page(PF, a, 1);
		}
	}
	
	lock_release(&mem_pool->lock);
}
