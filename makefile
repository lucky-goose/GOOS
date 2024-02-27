BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ -I thread/ -I userprog/
ASFLAGS = -f elf
#-fno-builtin告诉编译器不要用内部函数
#-Wstrict-prototypes要求函数声明必须有参数类型
#-Wmissing-prototypes要求函数必须有声明
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/thread.o \
		$(BUILD_DIR)/memory.o  $(BUILD_DIR)/console.o $(BUILD_DIR)/tss.o \
		$(BUILD_DIR)/process.o  \
		$(BUILD_DIR)/stdio.o $(BUILD_DIR)/syscallInit.o $(BUILD_DIR)/syscall.o\
		$(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o \
		$(BUILD_DIR)/bitmap.o $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o \
		$(BUILD_DIR)/sync.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o
#$<表示依赖中的第一个文件
#$@表示所有的目标文件
#$^表示所有的依赖文件
####	c代码编译	####

$(BUILD_DIR)/main.o : kernel/main.c lib/kernel/print.h \
						lib/stdint.h kernel/init.h kernel/debug.h \
						thread/thread.h device/console.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o : kernel/init.c kernel/init.h lib/kernel/print.h \
						lib/stdint.h kernel/interrupt.h device/timer.h \
						kernel/memory.h thread/thread.h device/console.h \
						device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/interrupt.o : kernel/interrupt.c kernel/interrupt.h \
							lib/kernel/print.h lib/stdint.h kernel/global.h \
							 lib/kernel/io.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/timer.o : device/timer.c device/timer.h lib/kernel/print.h \
					lib/stdint.h lib/kernel/io.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o : kernel/debug.c kernel/debug.h lib/kernel/print.h \
					lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/string.o : lib/string.c lib/string.h kernel/debug.h kernel/global.h \
					lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o : lib/kernel/bitmap.c lib/kernel/bitmap.h lib/string.h \
					lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/memory.o : kernel/memory.c kernel/memory.h lib/string.h \
					lib/stdint.h kernel/global.h lib/kernel/print.h \
					lib/kernel/bitmap.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/thread.o : thread/thread.c thread/thread.h lib/string.h \
					lib/stdint.h kernel/global.h \
					lib/kernel/print.h kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o : lib/kernel/list.c lib/kernel/list.h kernel/global.h \
					lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/sync.o : thread/sync.c thread/sync.h lib/kernel/list.h \
					lib/stdint.h thread/thread.h kernel/interrupt.h
					
	$(CC) $(CFLAGS) $< -o $@	
	
$(BUILD_DIR)/console.o : device/console.c device/console.h lib/kernel/print.h \
					lib/stdint.h thread/thread.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/keyboard.o : device/keyboard.c device/keyboard.h lib/kernel/print.h \
					lib/stdint.h lib/kernel/io.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o : device/ioqueue.c device/ioqueue.h thread/thread.h \
					lib/stdint.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/tss.o : userprog/tss.c userprog/tss.h thread/thread.h \
					lib/stdint.h kernel/global.h lib/kernel/print.h \
					lib/string.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/process.o : userprog/process.c userprog/process.h thread/thread.h \
					lib/stdint.h kernel/global.h  \
					userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o : lib/user/syscall.c lib/user/syscall.h lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/syscallInit.o : userprog/syscallInit.c userprog/syscallInit.h thread/thread.h \
					lib/stdint.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/stdio.o : lib/stdio.c lib/stdio.h lib/stdint.h lib/user/syscall.h
					
	$(CC) $(CFLAGS) $< -o $@

####	汇编代码编译		####
$(BUILD_DIR)/kernel.o : kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o : lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
	
$(BUILD_DIR)/switch.o : thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@

####	链接所有目标文件		####
$(BUILD_DIR)/kernel.bin : $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
	
.PHONY : hd clean all

hd:
	dd if=$(BUILD_DIR)/kernel.bin \
			of=/home/alex/bochs/hd60M.img \
			bs=512 count=200 seek=9 conv=notrunc;

clean:
	cd $(BUILD_DIR) && rm -f ./*
		
build: $(BUILD_DIR)/kernel.bin

all: build hd
