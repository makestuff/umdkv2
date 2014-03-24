#include <stdio.h>
#include <stdlib.h>
#include <libfpgalink.h>

int main(int argc, char *argv[]) {
	int retVal = 0;
	FILE *file = NULL;
	uint8 *romData = NULL;
	size_t romSize;
	const char *romFile;
	const char *dumpFile;
	uint8 line[5];
	size_t bytesRead;
	uint32 addr;
	uint16 readWord, romWord;
	if ( argc != 3 ) {
		fprintf(stderr, "Synopsis: logread <romFile> <dumpFile>\n");
		FAIL(1, cleanup);
	}
	romFile = argv[1];
	dumpFile = argv[2];
	romData = flLoadFile(romFile, &romSize);
	if ( !romData ) {
		fprintf(stderr, "Cannot load ROM image file \"%s\"!\n", romFile);
		FAIL(2, cleanup);
	}
	file = fopen(dumpFile, "rb");
	if ( !file ) {
		fprintf(stderr, "Cannot load dump file \"%s\"!\n", dumpFile);
		FAIL(3, cleanup);
	}
	bytesRead = fread(line, 1, 5, file);
	while ( bytesRead == 5 ) {
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
		bytesRead = fread(line, 1, 5, file);
	}
cleanup:
	if ( file ) {
		fclose(file);
	}
	flFreeFile(romData);
	return retVal;
}
