#include <stdio.h>
#include <stdlib.h>
#include <libfpgalink.h>

int main(void) {
	int retVal = 0;
	FILE *file = NULL;
	uint8 line[5];
	size_t bytesRead;
	uint32 addr;
	uint16 readWord, romWord;
	size_t romSize;
	uint8 *romData = flLoadFile("/home/chris/megadrive/sonic2.bin", &romSize);
	if ( !romData ) {
		fprintf(stderr, "Cannot load image file!\n");
		FAIL(1, cleanup);
	}
	file = fopen("dump.dat", "rb");
	if ( !file ) {
		fprintf(stderr, "Cannot load dump.dat!\n");
		FAIL(2, cleanup);
	}
	while ( !feof(file) ) {
		bytesRead = fread(line, 1, 5, file);
		addr = line[0] & 0x3f;
		addr <<= 8;
		addr |= line[1];
		addr <<= 8;
		addr |= line[2];
		addr <<= 1;
		readWord = line[3] << 8;
		readWord |= line[4];
		if ( addr >= romSize ) {
			printf("%01X %06X %04X XXXX\n", line[0] >> 6, addr, readWord);
		} else {
			romWord = romData[addr] << 8;
			romWord |= romData[addr+1];
			if ( romWord == readWord ) {
				printf("%01X %06X %04X %04X\n", line[0] >> 6, addr, readWord, romWord);
			} else {
				printf("%01X %06X %04X %04X *\n", line[0] >> 6, addr, readWord, romWord);
			}
		}
	}
cleanup:
	fclose(file);
	flFreeFile(romData);
	return retVal;
}
