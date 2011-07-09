TARGET = monitor
TOOLS = /usr/local/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf
AS_FLAGS = -D -mcpu=68000 -march=68000 --traditional-format --register-prefix-optional
#-alh

all: $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	$(TOOLS)-objcopy -O binary $< $@

$(TARGET).elf: $(TARGET).o
	$(TOOLS)-ld -nostdlib -Ttext 0x000000 -A 68000 --defsym _start=0 -o $@ $+

%.o: %.s
	$(TOOLS)-as $(AS_FLAGS) -o $@ $<

clean: FORCE
	rm -f *.o *.elf $(TARGET).bin

FORCE:
