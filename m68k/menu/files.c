#include "files.h"

struct FileEntry **dirList, **dirPtr;
static u16 *mcPtr;

void initFiles(u16 *freeMem) {
	dirList = (struct FileEntry **)freeMem;
	dirPtr = dirList;
	mcPtr = freeMem + 65536*sizeof(void*);
}

void storeFile(char *fileName, u16 fnLen, u32 firstCluster, u32 length) {
	if ( fnLen > 4 ) {
		const u16 *fn16 = (const u16 *)fileName;
		struct FileEntry *entry = (struct FileEntry *)mcPtr;
		u16 *ptr = (u16 *)&entry->fileName;
		char *ext = fileName + fnLen - 4;
		if ( ext[0] == '.' && ext[1] == 'b' && ext[2] == 'i' && ext[3] == 'n' ) {
			ext[0] = ext[1] = 0;
			fnLen -= 4;
			if ( fnLen > 36 ) {
				fileName[36] = 0;
				fnLen = 36;
			}
			if ( fnLen & 1 ) {
				fnLen++;
				fnLen >>= 1;
			} else {
				fnLen >>= 1;
				fnLen++;
			}
			while ( fnLen-- ) {
				*ptr++ = *fn16++;
			}
			entry->firstCluster = firstCluster;
			entry->length = length;
			mcPtr = ptr;
			*dirPtr++ = entry;
		}
	}
}

static inline short myStrCmp(const char *x, const char *y) {
	short retVal;
	while ( *x && *y && *x == *y ) {
		x++;
		y++;
	}
	retVal = *x;
	retVal -= *y;
	return retVal;
}

short myFileComp(CVPtr x, CVPtr y) {
	const struct FileEntry *u = (const struct FileEntry *)x;
	const struct FileEntry *v = (const struct FileEntry *)y;
	return myStrCmp(u->fileName, v->fileName);
}
