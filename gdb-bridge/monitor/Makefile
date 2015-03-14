TOOLS = $(HOME)/x-tools/m68k-megadrive-elf/bin/m68k-megadrive-elf
AS_FLAGS = -D -mcpu=68000 -march=68000 --bitwise-or --traditional-format --register-prefix-optional -L -al

all: test.bin monitor.bin

%.elf: %.o
	$(TOOLS)-ld -nostdlib -Ttext 0x000000 -A 68000 --defsym _start=0 -o $@ $+

monitor.bin: monitor.elf
	$(TOOLS)-objcopy -O binary $< $@
	@printf "\n\nMONITOR RESET ADDRESS IS $$(python -c "import sys; sys.stdout.write(hex((0x$$(nm monitor.elf | grep reset | awk '{print $$1}')+0x400000-0xf8)))"); YOU NEED TO CONFIRM THAT THE RESET ADDRESS\nIN ddd.sh IS THE SAME!\n\n\n"

%.bin: %.elf
	$(TOOLS)-objcopy -O binary $< $@

%.o: %.s
	$(TOOLS)-as $(AS_FLAGS) -o $@ $<

clean: FORCE
	rm -f *.o *.elf *.bin

FORCE:
