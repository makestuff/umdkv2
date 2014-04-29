#include <stdio.h>
#include <libdump.h>
#include "fat32.h"

void printFile(const char *fileName, u32 firstCluster, u32 firstBlock, u32 length) {
	//u8 buffer[BYTES_PER_SECTOR];
	printf("fileName: %s; firstCluster: 0x%08X; firstBlock: 0x%08X; length: 0x%08X\n", fileName, firstCluster, firstBlock, length);
	//sdReadBlocks(firstBlock, 1, buffer);
	//dump(0, buffer, BYTES_PER_SECTOR);
}

int main(void) {
	//u8 buffer[BYTES_PER_SECTOR];
	struct FileSystem fs;
	u8 retVal = fatOpenFileSystem(&fs);
	//u32 i;
	if ( retVal != FAT_SUCCESS ) goto cleanup;
	printf("fs.numSectorsPerCluster = %d\n", fs.numSectorsPerCluster);
	printf("fs.numSectorsPerFat = %d\n", fs.numSectorsPerFat);
	printf("fs.fatBeginAddress = 0x%08X\n", fs.fatBeginAddress);
	printf("FAT:\n");
	//for ( i = 0; i < fs.numSectorsPerFat; i++ ) {
	//	sdReadBlocks(0x820+i, 1, buffer);
	//	dump(i*BYTES_PER_SECTOR, buffer, BYTES_PER_SECTOR);
	//}
	printf("Directory:\n");
	fatListDirectory(&fs, fs.rootDirCluster, printFile);
cleanup:
	return retVal;
}
