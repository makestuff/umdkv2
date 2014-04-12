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
	uint8 line[9];
	size_t bytesRead;
	uint32 addr;
	uint16 readWord, romWord, flags;
	char charFlags[] = "    ";
	int newCount, oldCount;
	unsigned long long time = 0;
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
	bytesRead = fread(line, 1, 9, file);
	oldCount = line[0];
	oldCount <<= 8;
	oldCount |= line[1];
	oldCount <<= 8;
	oldCount |= line[2];
	oldCount <<= 6;
	oldCount |= line[3] >> 2;
	while ( bytesRead == 9 ) {
		newCount = line[0];
		newCount <<= 8;
		newCount |= line[1];
		newCount <<= 8;
		newCount |= line[2];
		newCount <<= 6;
		newCount |= line[3] >> 2;
		if ( oldCount > newCount ) {
			time += 0x40000000 + newCount - oldCount;
		} else {
			time += newCount - oldCount;
		}
		oldCount = newCount;
		flags = line[3] << 1;
		flags |= (line[4] >> 7);
		addr = line[4] & 0x7f;
		addr <<= 8;
		addr |= line[5];
		addr <<= 8;
		addr |= line[6];
		addr <<= 1;
		readWord = line[7] << 8;
		readWord |= line[8];
		charFlags[0] = (flags & 0x0004) ? 'D' : 'C';
		switch ( flags & 0x0003 ) {
			case 0: charFlags[2] = 'W'; charFlags[3] = 'B'; break;
			case 1: charFlags[2] = 'W'; charFlags[3] = 'H'; break;
			case 2: charFlags[2] = 'W'; charFlags[3] = 'L'; break;
			case 3: charFlags[2] = 'R'; charFlags[3] = 'D'; break;
		}
		if ( addr >= romSize ) {
			printf("%20llu %s %06X %04X XXXX\n", time, charFlags, addr, readWord);
		} else {
			romWord = romData[addr] << 8;
			romWord |= romData[addr+1];
			if ( romWord == readWord ) {
				printf("%20llu %s %06X %04X %04X\n", time, charFlags, addr, readWord, romWord);
			} else {
				printf("%20llu %s %06X %04X %04X *\n", time, charFlags, addr, readWord, romWord);
			}
		}
		bytesRead = fread(line, 1, 9, file);
	}
cleanup:
	if ( file ) {
		fclose(file);
	}
	flFreeFile(romData);
	return retVal;
}
