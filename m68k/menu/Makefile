SGDK_HOME := ../sgdk095
GCC_HOME := $(HOME)/x-tools/m68k-megadrive-elf
TOOLS = $(GCC_HOME)/bin/m68k-megadrive-elf
OPTS := -O1 -fomit-frame-pointer
CC_FLAGS := -m68000 -Wall $(OPTS) -fno-builtin-memset -fno-builtin-memcpy -I $(SGDK_HOME)/inc -I . -I $(SGDK_HOME)/res -c
ORIGIN := 0x000000

all: menu.bin

menu.bin: menu.elf
	$(TOOLS)-objcopy -O binary menu.elf menu.bin

menu.elf: sega.o main.o sdcard.o fat32.o files.o qsort.o
	$(TOOLS)-gcc -n -T $(SGDK_HOME)/md.ld -Ttext=$(ORIGIN) -e 0x200 -nostdlib $+ $(SGDK_HOME)/src/libmd.a $(GCC_HOME)/lib/gcc/m68k-megadrive-elf/4.3.4/libgcc.a -o menu.elf

%.o: %.s
	$(TOOLS)-as -D -mcpu=68000 -march=68000 --bitwise-or --traditional-format --register-prefix-optional -al $< -o $@

#	$(TOOLS)-gcc $(CC_FLAGS) $< -o $@

%.o: %.c
	$(TOOLS)-gcc $(CC_FLAGS) $< -o $@

gens: menu.bin
	LD_LIBRARY_PATH=/usr/lib/i386-linux-gnu/fglrx gens menu.bin 

clean: FORCE
	rm -rf *.o *.bin *.elf

FORCE:
