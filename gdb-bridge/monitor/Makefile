TOOLS = $(HOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf
AS_FLAGS = -D -mcpu=68000 -march=68000 --traditional-format --register-prefix-optional
#-alh

all: boot.bin monitor.bin test.bin

%.elf: %.o
	$(TOOLS)-ld -nostdlib -Ttext 0x000000 -A 68000 --defsym _start=0 -o $@ $+

%.bin: %.elf
	$(TOOLS)-objcopy -O binary $< $@

%.o: %.s
	$(TOOLS)-as $(AS_FLAGS) -o $@ $<

clean: FORCE
	rm -f *.o *.elf *.bin

FORCE:
