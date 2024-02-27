#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"
#include "stdint.h"

//获得结构体成员在结构体中的偏移量
#define offset(struct_type, member) (int)(&((struct_type*)0)->member)
//从结构体成员的地址获得结构体的地址
#define elem2entry(struct_type, struct_member_name, elem_ptr) (struct_type*)((uint32_t)elem_ptr - offset(struct_type, struct_member_name)) 

//双向链表的结点，因为我们只需要结点本身的地址，所以不需要数据成员
struct list_node {
	//前驱结点
	struct list_node* prev;
	//后继结点
	struct list_node* next;
};

//双向链表数据结构
struct list {
	struct list_node head;
	struct list_node tail;
};

//自定义函数类型function，用于在list_traversal中做回调函数
typedef uint32_t (function)(struct list_node* node, uint32_t args);

//初始化双向链表
void list_init(struct list* list);
//把node插入在before之前
void list_insert_before(struct list_node* before, struct list_node* node);
//把node结点插入到链表首
void list_push(struct list* list, struct list_node* node);
//把node结点插入到链表末端
void list_append(struct list* list, struct list_node* node);
//使node脱离链表
void list_remove(struct list_node* node);
//将链表第一个元素弹出并返回
struct list_node* list_pop(struct list* list);
//从链表中寻找node元素
uint32_t node_find(struct list* list, struct list_node* node);
//遍历链表所有元素，逐个判断是否有符合条件的元素
struct list_node* list_traversal(struct list* list, function func, uint32_t args);
//返回链表长度
uint32_t list_len(struct list* list);
//判断链表是否为空
bool list_empty(struct list* list);

#endif
