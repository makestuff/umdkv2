#include <stdio.h>
#include <libdump.h>
#include "fat32.h"

static u32 readLong(const u8 *ptr) {
	u32 val = ptr[3];
	val <<= 8;
	val |= ptr[2];
	val <<= 8;
	val |= ptr[1];
	val <<= 8;
	val |= ptr[0];
	return val;
}

static u16 readWord(const u8 *ptr) {
	u16 val = ptr[1];
	val <<= 8;
	val |= ptr[0];
	return val;
}

static void getFileName(const u8 *dirPtr, char *fileName) {
	u8 i = 9;
	while ( --i && *dirPtr != 0x20 ) {
		*fileName++ = *dirPtr++;
	}
	dirPtr += i;
	if ( *dirPtr != 0x20 ) {
		*fileName++ = '.';
		i = 2;
		do {
			*fileName++ = *dirPtr++;
		} while ( *dirPtr != 0x20 && i-- );
	}
	*fileName = '\0';
}

static u8 fatCacheBytes[BYTES_PER_SECTOR];
static u32 fatCacheAddress;
static u32 getNextCluster(const struct FileSystem *fs, u32 cluster) {
	u32 fatAddress;
	u16 fatOffset;
	fatAddress = fs->fatBeginAddress + (cluster * 4)/BYTES_PER_SECTOR;
	if ( fatAddress != fatCacheAddress ) {
		printf("Reading block 0x%08X into FAT cache\n", fatAddress);
		fatCacheAddress = fatAddress;
		sdReadBlocks(fatAddress, 1, fatCacheBytes);
		dump(0, fatCacheBytes, BYTES_PER_SECTOR);
	}
	fatOffset = (cluster * 4) % BYTES_PER_SECTOR;
	return readLong(fatCacheBytes + fatOffset);
}

static u8 getFirstPartitionAddress(u32 *address) {
	u8 buffer[BYTES_PER_SECTOR];
	sdReadBlocks(0, 1, buffer);
	//dump(0, buffer, BYTES_PER_SECTOR);
	if (
		(buffer[446+4] == 0x0C || buffer[446+4] == 0x0B) &&
		buffer[510] == 0x55 && buffer[511] == 0xAA ) 
	{
		*address = readLong(buffer+446+4+4);
		return FAT_SUCCESS;
	} else {
		return FAT_FORMAT;
	}
}

#define fatCheckLongNotZero() if ( readLong(ptr) == 0 ) { retVal = FAT_FORMAT; goto cleanup; } ptr += 4
#define fatCheckWord(expected) if ( readWord(ptr) != expected ) { retVal = FAT_FORMAT; goto cleanup; } ptr += 2
#define fatCheckByte(expected) if ( *ptr++ != expected ) { retVal = FAT_FORMAT; goto cleanup; }
#define NUM_FATS        2
#define FAT_DIRENT_SIZE 32

#define READONLY        (1<<0)
#define HIDDEN          (1<<1)
#define SYSTEM          (1<<2)
#define VOLUMEID        (1<<3)
#define DIRECTORY       (1<<4)
#define ARCHIVE         (1<<5)
#define LFN             (READONLY|HIDDEN|SYSTEM|VOLUMEID)

u8 fatOpenFileSystem(struct FileSystem *fs) {
	u32 vidAddress = 0;
	u16 numRsvdSectors;
	u8 retVal = 0;
	u8 buffer[BYTES_PER_SECTOR];
	const u8 *ptr = buffer + 11;
	fatCacheAddress = 0;
	sdInit();
	retVal = getFirstPartitionAddress(&vidAddress);
	if ( retVal != FAT_SUCCESS ) goto cleanup;
	sdReadBlocks(vidAddress, 1, buffer);
	//dump(0, buffer, BYTES_PER_SECTOR);
	fatCheckWord(BYTES_PER_SECTOR);                 // Word @ offset 11: BPB_BytsPerSec
	fs->numSectorsPerCluster = *ptr++;              // Byte @ offset 13: BPB_SecPerClus
	numRsvdSectors = readWord(ptr); ptr += 2;       // Word @ offset 14: BPB_RsvdSecCnt
	fatCheckByte(NUM_FATS);                         // Byte @ offset 16: BPB_NumFATs
	fatCheckWord(0x0000);                           // Word @ offset 17: BPB_RootEntCount
	fatCheckWord(0x0000);                           // Word @ offset 19: BPB_TotSec16
	ptr++;                                          // Byte @ offset 21: BPB_Media
	fatCheckWord(0x0000);                           // Word @ offset 22: BPB_FATSz16
	ptr += 2+2+4;                                   // 8    @ offset 24: BPB_SecPerTrk, BPB_NumHeads & BPB_HiddSec
	fatCheckLongNotZero();                          // Long @ offset 32: BPB_TotSec
	fs->numSectorsPerFat = readLong(ptr); ptr += 4; // Long @ offset 36: BPB_FATSz32
	ptr += 4;                                       // Long @ offset 40: BPB_ExtFlags & BPB_FSVer
	fs->rootDirCluster = readLong(ptr); ptr += 4;   // Long @ offset 44: BPB_RootClus
	ptr += 510-48;
	fs->fatBeginAddress = vidAddress + numRsvdSectors;
	fatCheckWord(0xAA55);
cleanup:
	return retVal;
}

#define fatGetNumBytesPerCluster(fs) (fs->numSectorsPerCluster * BYTES_PER_SECTOR)
#define fatIsEndOfClusterChain(thisCluster) (thisCluster == 0x0FFFFFFF)
#define fatGetClusterAddress(fs, cluster) (fs->fatBeginAddress + (NUM_FATS * fs->numSectorsPerFat) + (cluster - 2) * fs->numSectorsPerCluster)
#define fatIsDeletedFile(dirPtr) (*dirPtr == 0xE5)
#define fatIsLongFilename(dirPtr) ((dirPtr[11] & LFN) == LFN)
#define fatIsVolumeName(dirPtr) (dirPtr[11] & VOLUMEID)
#define fatGetFileSize(dirPtr) readLong(dirPtr+28)
#define fatGetFirstCluster(dirPtr) ((readWord(dirPtr+20)<<16) + readWord(dirPtr+26))

u8 fatListDirectory(struct FileSystem *fs, u32 dirCluster, FileNameCallback callback) {
	const u16 entriesPerCluster = fatGetNumBytesPerCluster(fs)/FAT_DIRENT_SIZE;
	char fileName[13]; // THISFILE.TXT\0: 8+1+3+1
	u8 buffer[fatGetNumBytesPerCluster(fs)];
	const u8 *ptr = buffer;
	u16 i = 0;
	u8 retVal = 0;
	u32 firstCluster;
	sdReadBlocks(fatGetClusterAddress(fs, dirCluster), fs->numSectorsPerCluster, buffer);
	for ( ; ; ) {
		if ( *ptr == 0x00 ) {
			break;  // no more directory entries, so quit
		}
		printf("%d: %s %s\n", i, fatIsDeletedFile(ptr)?"isDeleted":"notDeleted", fatIsLongFilename(ptr)?"isLong":"notLong");
		//dump(0, ptr, FAT_DIRENT_SIZE);
		if ( !fatIsDeletedFile(ptr) && !fatIsLongFilename(ptr) ) {
			getFileName(ptr, fileName);
			if ( !fatIsVolumeName(ptr) ) {
				firstCluster = fatGetFirstCluster(ptr);
				callback(fileName, firstCluster, fatGetClusterAddress(fs, firstCluster), fatGetFileSize(ptr));
			}
		}
		ptr += FAT_DIRENT_SIZE;
		i++;
		if ( i == entriesPerCluster ) {
			dirCluster = getNextCluster(fs, dirCluster);
			if ( fatIsEndOfClusterChain(dirCluster) ) {
				break;
			}
			sdReadBlocks(fatGetClusterAddress(fs, dirCluster), fs->numSectorsPerCluster, buffer);
			i = 0;
			ptr = buffer;
		}
	}
	return retVal;
}
