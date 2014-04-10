unsigned short vtimer;
unsigned short htimer;
unsigned short p[8];

void save_it(unsigned short x);
unsigned short calc1(unsigned short x);
unsigned short my_func(unsigned short x, unsigned short y, unsigned short z);

int main(void) {
	unsigned short i;
	unsigned short x;
	p[0] = 0x0023;
	p[1] = 0xCAFE;
	p[2] = 0xBABE;
	p[3] = 0xF00D;
	p[4] = 0x1234;
	p[5] = 0x5678;
	p[6] = 0xABCD;
	p[7] = 0xB00B;
	i = 10;
	for ( ; ; ) {
		x = (i ^ 0xDEAD) + 1;
		x = calc1(x);
		save_it(x);
		*((unsigned short *)0xFF0002) = my_func(10, 20, 30);
		i++;
	}
	return 0;
}

void hblank(void) { }

void vblank(void) { }
