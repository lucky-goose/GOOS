#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"

//键盘中断处理程序
void intr_keyboard_handler(void) {
	//这次中断前的中断，以下三个键是否有按下
	bool ctrl_down_last = ctrl_status;
	bool shift_down_last = shift_status;
	bool caps_lock_last = caps_lock_status;
	
	bool break_code;
	uint16_t scancode = inb(KBD_BUF_PORT);
	
	//如果此次扫描码为0xeo，则表示扩展，直接返回
	if (scancode == 0xe0) {
		ext_scancode = true;
		return;
	}
	
	//如果上次扫描码为0xe0,则合并
	if (ext_scancode == true) {
		scancode = (0xe000 | scancode);
		ext_scancode = false;
	}
	
	//判断是否为断码
	break_code = ((scancode & 0x0080) != 0);
	
	//如果是断码
	if (break_code) {
		//通过断码得到其扫描码
		uint16_t make_code = (scancode &= 0xff7f);
		
		//如果以下三个键松开了，则设置对应状态为false
		if (make_code == ctrl_l_make || make_code == ctrl_r_make) { 
			ctrl_status = false;
		} else if (make_code == shift_l_make || make_code == shift_r_make) {
			shift_status = false;
		} else if (make_code == alt_l_make || make_code == alt_r_make) {
			alt_status = false;
		} 
		return;
	}
	//若为通码
	if ((scancode > 0x00 && scancode < 0x3b) || \
		(scancode == alt_r_make) || \
		(scancode == ctrl_r_make)) {
		//用来判断是否与shift组合，将来用于在一维数组中索引对应的字符
		bool shift = false;
		
		//如果是表示两个字符的键，例如'0'~'9'
		if ((scancode < 0x0e) || (scancode == 0x29) || \
			(scancode == 0x1a) || (scancode == 0x1b) || \
			(scancode == 0x2b) || (scancode == 0x27) || \
			(scancode == 0x28) || (scancode == 0x33) || \
			(scancode == 0x34) || (scancode == 0x35)) {
				//表示两个字符的键的具体表示只与shift的状态有关				
				if (shift_down_last) shift = true;
		} else {
			//如果是字母键,只有shift和caps按下其中一个才表示大写
			shift = shift_down_last ^ caps_lock_last;
		}
		
		//一律当作非扩展处理
		uint8_t index = (scancode &= 0x00ff);
		uint8_t cur_char = keymap[index][shift];
		
		//只打印ascii码不为0的键
		if (cur_char) {
			if (!ioq_full(&kbd_buf)) {
				ioq_putchar(&kbd_buf, cur_char);
			}
			return;
		}
		
		//判断此次按下的是否为以下控制键
		if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
			ctrl_status = true;
		} else if (scancode == shift_l_make || scancode == shift_r_make) {
			shift_status = true;
		} else if (scancode == alt_l_make || scancode == alt_r_make) {
			alt_status = true;
		} else if (scancode == caps_lock_make) {
			caps_lock_status ^= 1;
		}
	} else {
		put_str("unknown key\n");
	}
}

//键盘初始化
void keyboard_init(void) {
	put_str("keyboard init start\n");
	ioqueue_init(&kbd_buf);
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done\n");
}
