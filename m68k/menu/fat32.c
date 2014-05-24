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

static u8 fatCacheBytes[BYTES_PER_SECTOR];
static u32 fatCacheAddress;
static u32 getNextCluster(const struct FileSystem *fs, u32 cluster) {
	u32 fatAddress;
	u16 fatOffset;
	fatAddress = fs->fatBeginAddress + (cluster * 4)/BYTES_PER_SECTOR;
	if ( fatAddress != fatCacheAddress ) {
		fatCacheAddress = fatAddress;
		sdReadBlocks(fatAddress, 1, fatCacheBytes);
	}
	fatOffset = (cluster * 4) % BYTES_PER_SECTOR;
	return readLong(fatCacheBytes + fatOffset);
}

static u8 getFirstPartitionAddress(u32 *address) {
	u8 buffer[BYTES_PER_SECTOR];
	sdReadBlocks(0, 1, buffer);
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
	const u16 bytesPerCluster = fatGetNumBytesPerCluster(fs);
	char fileName[20*13+2];
	u8 buffer[bytesPerCluster];
	const u8 *ptr = buffer;
	u16 i = 0;
	u8 retVal = 0;
	u32 firstCluster;
	u8 offset, last;
	u16 fnLen = 0;
	sdReadBlocks(fatGetClusterAddress(fs, dirCluster), fs->numSectorsPerCluster, buffer);
	for ( ; ; ) {
		if ( *ptr == 0x00 ) {
			break;  // no more directory entries, so quit
		} else if ( fatIsLongFilename(ptr) ) {
			offset = *ptr;
			last = offset & 0x40;
			offset &= 0x1F;
			offset--;
			offset *= 13;
			fileName[offset+0] = ptr[1];
			fileName[offset+1] = ptr[3];
			fileName[offset+2] = ptr[5];
			fileName[offset+3] = ptr[7];
			fileName[offset+4] = ptr[9];
			fileName[offset+5] = ptr[14];
			fileName[offset+6] = ptr[16];
			fileName[offset+7] = ptr[18];
			fileName[offset+8] = ptr[20];
			fileName[offset+9] = ptr[22];
			fileName[offset+10] = ptr[24];
			fileName[offset+11] = ptr[28];
			fileName[offset+12] = ptr[30];
			if ( last ) {
				fileName[offset+13] = '\0';
				fileName[offset+14] = '\0';
				offset += 12;
				while ( fileName[offset] == 0 || fileName[offset] == -1 ) {
					fileName[offset] = 0;
					offset--;
				}
				offset++;
				fnLen = offset;
			}
		} else if ( !fatIsDeletedFile(ptr) && !fatIsVolumeName(ptr) ) {
			firstCluster = fatGetFirstCluster(ptr);
			callback(fileName, fnLen, firstCluster, fatGetFileSize(ptr));
			fnLen = 0;
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

u32 fatGetClusterLength(struct FileSystem *fs) {
	u32 len = fs->numSectorsPerCluster;
	len *= BYTES_PER_SECTOR;
	return len;
}

u32 fatReadCluster(struct FileSystem *fs, u32 cluster, u8 *buffer) {
	sdReadBlocks(fatGetClusterAddress(fs, cluster), fs->numSectorsPerCluster, buffer);
	return getNextCluster(fs, cluster);
}
