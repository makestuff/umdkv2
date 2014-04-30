#include <stdio.h>
#include <libdump.h>
#include "fat32.h"
#include "qsort.h"

void printFile(const char *fileName, u32 firstCluster, u32 firstBlock, u32 length) {
	//u8 buffer[BYTES_PER_SECTOR];
	printf("fileName: %s; firstCluster: 0x%08X; firstBlock: 0x%08X; length: 0x%08X\n", fileName, firstCluster, firstBlock, length);
	//sdReadBlocks(firstBlock, 1, buffer);
	//dump(0, buffer, BYTES_PER_SECTOR);
}

int main(void) {
	int array[] = {3, 8, 2, 5, 1, 4, 7, 6};
	const u16 numElems = sizeof(array)/sizeof(*array);
	u16 i;
	randInit(0xF133, 0x2153, 0x770D, 0x416B);
	quickSort(array, 0, numElems);
	printf("Sorted array: {%d", array[0]);
	for ( i = 1; i < numElems; i++ ) {
		printf(", %d", array[i]);
	}
	printf("}\n");
	return 0;
}

int main2(void) {
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
