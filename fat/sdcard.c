#include <stdio.h>
#include <stdlib.h>
#include "sdcard.h"

static FILE *file = NULL;

void sdInit(void) {
	file = fopen("sdAll.bin", "rb");
	if ( !file ) {
		fprintf(stderr, "Unable to open sdAll.bin!\n");
		exit(1);
	}
}

void sdReadBlocks(u32 blockStart, u16 blockCount, u8 *buffer) {
	const long byteOffset = blockStart * BYTES_PER_SECTOR;
	fseek(file, byteOffset, SEEK_SET);
	fread(buffer, BYTES_PER_SECTOR, blockCount, file);
}
