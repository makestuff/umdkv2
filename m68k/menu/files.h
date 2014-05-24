#ifndef FILES_H
#define FILES_H

#include "sdcard.h"
#include "qsort.h"

struct FileEntry {
	u32 firstCluster;
	u32 length;
	char fileName[1];
};
extern struct FileEntry **dirList, **dirPtr;
void initFiles(u16 *freeMem);
void storeFile(char *fileName, u16 fnLen, u32 firstCluster, u32 length);
short myFileComp(CVPtr x, CVPtr y);

#endif
