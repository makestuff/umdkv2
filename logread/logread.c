#include <stdio.h>
#include <stdlib.h>
#include <libfpgalink.h>

typedef enum {
	WB = 0,  // write both bytes
	WH,      // write high byte
	WL,      // write low byte
	RD,      // read
	HB       // heartbeat
} BusType;

typedef enum {
	DMA = 0,
	CPU
} BusSrc;

static inline BusType getType(const uint8 *line) {
	return (BusType)(line[0] >> 5);
}

static inline int getTimeStamp(const uint8 *line) {
	int count = line[0] & 0x1F;
	count <<= 8;
	count |= line[1];
	return count;
}

static inline BusSrc getSrc(const uint8 *line) {
	return (line[4] & 0x01) ? DMA : CPU;
}

static inline uint32 getAddr(const uint8 *line) {
	uint32 addr = line[2];
	addr <<= 8;
	addr |= line[3];
	addr <<= 8;
	addr |= line[4];
	return addr;
}

static inline uint16 getWord(const uint8 *line) {
	uint16 data = line[5];
	data <<= 8;
	data |= line[6];
	return data;
}

int main(int argc, char *argv[]) {
	int retVal = 0;
	FILE *file = NULL;
	uint8 *romData = NULL;
	size_t romSize = 0;
	const char *dumpFile;
	uint8 line[7];
	size_t bytesRead;
	BusType busType;
	BusSrc busSrc;
	uint32 busAddr;
	uint16 busWord, romWord;
	char charFlags[] = "    ";
	int newTS, oldTS;
	unsigned long long time = 0;
	if ( argc < 2 || argc > 3 ) {
		fprintf(stderr, "Synopsis: logread <dumpFile> [<romFile>]\n");
		FAIL(1, cleanup);
	}
	dumpFile = argv[1];
	if ( argc == 3 ) {
		const char *romFile = argv[2];
		romData = flLoadFile(romFile, &romSize);
		if ( !romData ) {
			fprintf(stderr, "Cannot load ROM image file \"%s\"!\n", romFile);
			FAIL(2, cleanup);
		}
	}
	file = fopen(dumpFile, "rb");
	if ( !file ) {
		fprintf(stderr, "Cannot load dump file \"%s\"!\n", dumpFile);
		FAIL(3, cleanup);
	}
	bytesRead = fread(line, 1, 7, file);
	oldTS = 0;
	while ( bytesRead == 7 ) {
		newTS = getTimeStamp(line);
		if ( oldTS > newTS ) {
			time += 8192 + newTS - oldTS;
		} else {
			time += newTS - oldTS;
		}
		oldTS = newTS;
		busType = getType(line);
		newTS = getTimeStamp(line);
		busSrc = getSrc(line);
		busAddr = getAddr(line);
		busWord = getWord(line);
		switch ( busSrc ) {
			case DMA: charFlags[0] = 'D'; break;
			case CPU: charFlags[0] = 'C'; break;
		};
		switch ( busType ) {
			case WB: charFlags[2] = 'W'; charFlags[3] = 'B'; break;
			case WH: charFlags[2] = 'W'; charFlags[3] = 'H'; break;
			case WL: charFlags[2] = 'W'; charFlags[3] = 'L'; break;
			case RD: charFlags[2] = 'R'; charFlags[3] = 'D'; break;
			default: break;
		}
		if ( busType == HB ) {
			printf("%20llu HEARTBEAT\n", time);
		} else {
			if ( romData ) {
				if ( busAddr >= romSize ) {
					printf("%20llu %s %06X %04X XXXX\n", time, charFlags, busAddr, busWord);
				} else {
					romWord = romData[busAddr] << 8;
					romWord |= romData[busAddr+1];
					if ( romWord == busWord ) {
						printf("%20llu %s %06X %04X %04X\n", time, charFlags, busAddr, busWord, romWord);
					} else {
						printf("%20llu %s %06X %04X %04X *\n", time, charFlags, busAddr, busWord, romWord);
					}
				}
			} else {
				printf("%20llu %s %06X %04X\n", time, charFlags, busAddr, busWord);
			}
		}
		bytesRead = fread(line, 1, 7, file);
	}
cleanup:
	if ( file ) {
		fclose(file);
	}
	flFreeFile(romData);
	return retVal;
}
