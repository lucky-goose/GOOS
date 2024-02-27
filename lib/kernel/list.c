#include "list.h"
#include "interrupt.h"
//初始化双向链表
void list_init(struct list* list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

//把node插入在before之前
void list_insert_before(struct list_node* before, struct list_node* node) {
	//关闭中断
	enum intr_status old_status = intr_disable();
	
	//修改结点指针
	before->prev->next = node;
	
	node->prev = before->prev;
	node->next = before;
	
	before->prev = node;
	
	//恢复中断
	set_intr_status(old_status);
}

//把node结点插入到链表首
void list_push(struct list* list, struct list_node* node) {
	list_insert_before(list->head.next, node);
}

//把node结点插入到链表末端
void list_append(struct list* list, struct list_node* node) {
	list_insert_before(&list->tail, node);
}

//使node脱离链表
void list_remove(struct list_node* node) {
	//关闭中断
	enum intr_status old_status = intr_disable();
	
	node->prev->next = node->next;
	node->next->prev = node->prev;
	
	//恢复中断
	set_intr_status(old_status);
}

//将链表第一个元素弹出并返回
struct list_node* list_pop(struct list* list) {
	struct list_node* top = list->head.next;
	list_remove(top);
	return top;
}

//从链表中寻找node元素
uint32_t node_find(struct list* list, struct list_node* node) {
	struct list_node* curr = list->head.next;
	//遍历链表并比较
	while (curr != &list->tail) {
		if (curr == node) {
			return 1;
		}
		curr = curr->next;
	}
	return 0;
}

//遍历链表所有元素，逐个判断是否有符合条件的元素
struct list_node* list_traversal(struct list* list, function func, uint32_t args) {
	struct list_node* curr = list->head.next;
	while (curr != &list->tail) {
		if (func(curr, args)) {
			//如果curr符合回调函数的条件，则返回
			return curr;
		}
		curr = curr->next;
	}
	return NULL;
}

//返回链表长度
uint32_t list_len(struct list* list) {
	struct list_node* curr = list->head.next;
	uint32_t len = 0;
	while (curr != &list->tail) {
		len++;
		curr = curr->next;
	}
	return len;
}

//判断链表是否为空
bool list_empty(struct list* list) {
	return (list->head.next == &list->tail ? 1 : 0);
}
