void save_it(unsigned short x) {
	*((unsigned short *)0xFF0000) = x;
}
