TARGET = testrom
TOOLS = /home/chris/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf
AS_FLAGS = -D -mcpu=68000 -march=68000 --traditional-format --register-prefix-optional
# -alh

all: $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	$(TOOLS)-objcopy -O binary $< $@

$(TARGET).elf: crt0.o $(TARGET).o
	$(TOOLS)-ld -nostdlib -Ttext 0x000000 -A 68000 --defsym _start=0 -o $@ $+

%.o: %.s
	$(TOOLS)-as $(AS_FLAGS) -o $@ $<

clean: FORCE
	rm -f crt0.s *.o *.elf $(TARGET).bin

crt0.s:
	wget -nv -O crt0.s 'http://www.megadrive.org/cvsweb.cgi/libmd/crt0.s?rev=HEAD;content-type=text%2Fplain'

FORCE:
