#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H
#include "stdint.h"
#include "ioqueue.h"

#define KBD_BUF_PORT 0x60	//键盘buffer寄存器端口号为0x60

//用转移字符定义部分控制字符(8进制）
#define esc 		'\033'
#define backspace	'\b'
#define tab			'\t'
#define enter		'\r'
#define delete		'\0177'

//以下控制字符不可见，一律定义为0
#define char_invisible	0
#define ctrl_l_char		char_invisible
#define ctrl_r_char		char_invisible
#define shift_l_char	char_invisible
#define shift_r_char	char_invisible
#define alt_l_char		char_invisible
#define alt_r_char		char_invisible
#define caps_lock_char	char_invisible

//定义控制字符的通码和段码
#define shift_l_make 0x2a 
#define shift_r_make 0x36
#define alt_l_make   0x38
#define alt_r_make   0xe038
#define alt_r_break  0xe0b8
#define ctrl_l_make  0x1d
#define ctrl_r_make  0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

//定义键盘缓冲区
struct ioqueue kbd_buf;

//定义以下变量记录相应健是否按下,ext_scancode用于记录makecode是否以0xe0开头
bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

//以通码make_code为索引的二维数组
static char keymap[][2] = {
	{0, 0},
	{esc, esc},
	{'1', '!'},
	{'2', '@'},
	{'3', '#'},
	{'4', '$'},
	{'5', '%'},
	{'6', '^'},
	{'7', '&'},
	{'8', '*'},
	{'9', '('},	
	{'0', ')'},
	{'-', '_'},
	{'=', '+'},
	{backspace, backspace},
	{tab, tab},
	{'q', 'Q'},
	{'w', 'W'},
	{'e', 'E'},
	{'r', 'R'},
	{'t', 'T'},
	{'y', 'Y'},
	{'u', 'U'},
	{'i', 'I'},
	{'o', 'O'},
	{'p', 'P'},
	{'[', '{'},
	{']', '}'},
	{enter, enter},
	{ctrl_l_char, ctrl_l_char},
	{'a', 'A'},
	{'s', 'S'},
	{'d', 'D'},
	{'f', 'F'},
	{'g', 'G'},
	{'h', 'H'},
	{'j', 'J'},
	{'k', 'K'},
	{'l', 'L'},
	{';', ':'},
	{'\'', '"'},
	{'`', '~'},
	{shift_l_char, shift_l_char},
	{'\\', '|'},
	{'z', 'Z'},
	{'x', 'X'},
	{'c', 'C'},
	{'v', 'V'},
	{'b', 'B'},
	{'n', 'N'},
	{'m', 'M'},
	{',', '<'},
	{'.', '>'},
	{'/', '?'},
	{shift_r_char, shift_r_char},
	{'*', '*'},
	{alt_l_char, alt_l_char},
	{' ', ' '},
	{caps_lock_char, caps_lock_char}
	//其他键暂不处理
};

//键盘中断处理程序
void intr_keyboard_handler(void);
//键盘初始化
void keyboard_init(void);

#endif
