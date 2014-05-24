#ifndef FAT32_H
#define FAT32_H

#include "sdcard.h"

struct FileSystem {
	u8 numSectorsPerCluster;
	u32 numSectorsPerFat;
	u32 rootDirCluster;
	u32 fatBeginAddress;
};

typedef void (*FileNameCallback)(char *, u16, u32, u32);

enum {
	FAT_SUCCESS,
	FAT_FORMAT
};

u8 fatOpenFileSystem(struct FileSystem *fs);
u8 fatListDirectory(struct FileSystem *fs, u32 dirCluster, FileNameCallback callback);
u32 fatGetClusterLength(struct FileSystem *fs);
u32 fatReadCluster(struct FileSystem *fs, u32 cluster, u8 *buffer);

#endif
