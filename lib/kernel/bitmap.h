#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "stdint.h"

#define BITMAP_MASK 1

struct bitmap {
	//位图的长度，以字节为单位
	uint32_t btmp_bytes_len;
	//位图数组的首地址
	uint8_t* bits;
};

//初始化位图
void bitmap_init(struct bitmap* btmp);
//判断位图的某个位是否为1
int bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
//在位图中寻找连续的cnt个位，找到则返回起始下标，否则返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
//设置bitmap的某个位的值为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);

#endif
