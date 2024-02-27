#include "bitmap.h"
#include "string.h"
#include "debug.h"

//初始化位图
void bitmap_init(struct bitmap* btmp) {
	//全部初始化为0
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

//判断位图的某个位是否为1
int bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
	//索引数组下标
	uint32_t byte_idx = bit_idx / 8;
	//索引字节内的位
	uint32_t bit_off = bit_idx % 8;
	return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_off));
}

//在位图中寻找连续的cnt个位，找到则返回起始下标，否则返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
	uint32_t byte_idx = 0;
	//先找到第一个有0的byte
	while ((byte_idx < btmp->btmp_bytes_len) && (0xff == btmp->bits[byte_idx])) {
		byte_idx++;
	}
	ASSERT(byte_idx < btmp->btmp_bytes_len);
	//如果没找到则返回-1
	if (byte_idx >= btmp->btmp_bytes_len) return -1;
	
	uint32_t bit_off = 0;
	//找出该字节中第一个空位的下标
	while (btmp->bits[byte_idx] & (BITMAP_MASK << bit_off)) bit_off++;
	
	//第一个空闲位在位图中的下标
	uint32_t bit_idx = byte_idx * 8 + bit_off;
	//所有的bit数
	uint32_t btmp_bits_len = btmp->btmp_bytes_len * 8;
	//当前连续的空位数
	uint32_t count = 0;
	//连续cnt个空位开始的下标
	uint32_t bit_idx_start = -1;
	
	//寻找连续cnt个空位
	for (; bit_idx < btmp_bits_len; bit_idx++) {
		if (!bitmap_scan_test(btmp, bit_idx)) {
			count++;
		} else {
			count = 0;
		}
		if (count == cnt) {
			bit_idx_start = bit_idx - cnt + 1;
			break;
		}
	} 
	
	return bit_idx_start;
}

//设置bitmap的某个位的值为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
	ASSERT(value == 0 || value == 1);
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_off = bit_idx % 8;
	if (value) {
		btmp->bits[byte_idx] |= (BITMAP_MASK << bit_off);
	} else {
		btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_off);
	}
}
