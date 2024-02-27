#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "sync.h"
#include "list.h"

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

//内存块描述符个数
#define MEM_BLOCK_DESC_CNT 7

struct virtual_addr_pool{
	//虚拟地址位图
	struct bitmap vaddr_bitmap;
	//虚拟地址起始地址
	uint32_t vaddr_start;
};

struct mem_pool {
	//内存池位图
	struct bitmap mem_pool_bitmap;
	//物理地址起始
	uint32_t phy_addr_start;
	//物理内存的大小（单位字节）
	uint32_t pool_size;
	//申请内存时互斥
	struct lock lock;
};

//内存池标记
enum pool_flags {
	PF_KERNEL = 1,	//内核内存池
	PF_USER = 2	//用户内存池
};

//内存块
struct mem_block {
	struct list_node free_block;
};

//内存块描述符
//内存块描述符共有七个，分别为16,32,64,128,256,512,1024
struct mem_block_desc {
	uint32_t block_size;
	uint32_t blocks_per_arena;
	struct list free_list;
};

struct arena {
	//此arena所属的mem_block_desc
	struct mem_block_desc* desc;
	//当large为true时，cnt表示页框数
	//当large为false时，cnt表示空闲的mem_block数
	uint32_t cnt;
	bool large;
};

struct mem_pool kernel_mem_pool, user_mem_pool;
struct virtual_addr_pool kernel_vaddr_pool;
struct mem_block_desc k_block_descs[MEM_BLOCK_DESC_CNT];

//初始化内存池
void mem_pool_init(uint32_t all_mem);
//初始化内存管理系统
void mem_init(void);
//在虚拟内存池中申请pg_cnt个虚拟页，成功则返回起始地址，失败则返回NULL
void* get_virtual_addr(enum pool_flags pf, uint32_t pg_cnt);
//得到虚拟地址对应的pte指针
uint32_t* pte_ptr(uint32_t vaddr);
//得到虚拟地址对应的pde指针
uint32_t* pde_ptr(uint32_t vaddr);
//在m_pool指向的内存池中分配1个物理页，成功则返回物理地址，失败则返回NULL
void* palloc(struct mem_pool* m_pool);
//在页表中添加_vaddr到_paddr的映射
void page_table_map(void* _vaddr, void* _paddr);
//分配pg_cnt个页空间，成功则返回起始虚拟地址，失败则返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
//从内核物理内存池中申请pg_cnt页内存，成功则返回其虚拟地址，失败则返回NULL
void* get_kernel_pages(uint32_t pg_cnt);
//在用户内存空间中申请4k内存，并返回其虚拟地址
void* get_user_pages(uint32_t pg_cnt);
//分配一页内存，并将其和指定的虚拟地址关联
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
//通过虚拟地址得到其物理地址
uint32_t addr_v2p(uint32_t vaddr);
//为malloc做准备
void block_desc_init(struct mem_block_desc* desc_array);
//返回arena中第idx个内存块的地址
struct mem_block* arena2block(struct arena* a, uint32_t idx);
//返回内存块b所在的arena的地址
struct arena* block2arena(struct mem_block* b);
//在堆中申请size字节内存
void* sys_malloc(uint32_t size);
//将物理地址pg_phy_addr回收到物理内存池
void pfree(uint32_t pg_phy_addr);
//去掉页表中虚拟地址vaddr的映射，只去掉addr对应的pte
void page_table_pte_remove(uint32_t vaddr);
//在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟地址
void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);
//释放以虚拟地址addr起始的cnt个物理页
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);
//回收内存ptr
void sys_free(void* ptr);
#endif
