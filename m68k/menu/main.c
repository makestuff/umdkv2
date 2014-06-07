#include "genesis.h"
#include "fat32.h"
#include "qsort.h"
#include "files.h"
#include "sdcard.h"

void doSelect(struct FileSystem *fs, s16 choice) {
	const struct FileEntry *const file = dirList[choice];
	const char *str = file->fileName;
	u32 cluster = file->firstCluster;
	u32 length = file->length;
	const u32 clusterLen = fatGetClusterLength(fs);
	const u16 clustersPerStar = (u16)(length / (32*clusterLen));
	u16 x = 0, i;
	volatile u8 *const ssf2Reg = (volatile u8 *)0xA130F3;
	//u8 page = 0x48;
	u8 page = 0x40;
	u8 *const LWM = (u8*)0x480000;
	u8 *const HWM = LWM + 512*1024;
	u8 *ptr = LWM;
	u8 isFirst;

	// Work out what x offset to use for the ROM name
	while ( *str ) {
		str++;
		x++;
	}
	x = (40-x) >> 1;
	str = dirList[choice]->fileName;

	// Construct loading screen
	VDP_waitVSync();
	VDP_clearPlan(VDP_PLAN_A, 1);
	VDP_waitDMACompletion();
	VDP_setTextPalette(PAL1);
	VDP_drawText(str, x, 11);
	VDP_setTextPalette(PAL2);
	VDP_drawText("Loading...", 15, 24);
	VDP_drawText("+--------------------------------+", 3, 25);
	VDP_drawText("|                                |", 3, 26);
	VDP_drawText("+--------------------------------+", 3, 27);
	VDP_setTextPalette(PAL0);
	*ssf2Reg = page;
	isFirst = 1;
	*((u16*)0x430000) = (clusterLen>>2)-1;
	for ( x = 4; x < 32+4; x++ ) {
		for ( i = 0; i < clustersPerStar; i++ ) {
			if ( isFirst ) {
				cluster = fatReadCluster(fs, cluster, (u8*)0x430002);
				isFirst = 0;
			} else {
				cluster = fatReadCluster(fs, cluster, ptr);
			}
			ptr += clusterLen;
			if ( ptr == HWM ) {
				ptr = LWM;
				page++;
				*ssf2Reg = page;
			}
		}
		VDP_drawText("*", x, 26);
	}
	*ssf2Reg = 0x40; // point back at bottom of RAM
	__asm__("trap #0");
}

int main() {
	u16 i, newJoy = 0, oldJoy = 0, autoRepeatDelay = 0, redraw = 1;
	s16 choice = 0;
	const char *str;
	struct FileSystem fs;
	u16 numFiles;

	// Init the screen, display message
	VDP_setScreenWidth320();
	VDP_setHInterrupt(0);
	VDP_setHilightShadow(0);
	VDP_setTextPalette(PAL0);
	VDP_drawText("MakeStuff USB MegaDrive Dev Kit v2", 3, 10);
	VDP_drawText("Reading SD-card...", 11, 12);

	// Initialise the SD card
	sdInit();

	// Get the geometry of the first SD-card partition
	fatOpenFileSystem(&fs);

	// Initialise workspace for the directory list
	initFiles((u16*)0x440000);

	// Get the list of game ROMs
	fatListDirectory(&fs, fs.rootDirCluster, storeFile);

	// Sort the list alphabetically
	numFiles = dirPtr - dirList;
	quickSort((CVPtr *)dirList, 0, numFiles, (CompareFunc)myFileComp);

	// Display the list
	for ( ; ; ) {
		newJoy = JOY_readJoypad(0);
		if ( newJoy & BUTTON_UP && choice > 0 ) {
			if ( !(oldJoy & BUTTON_UP) ) {
				choice--; redraw = 1;
				autoRepeatDelay = 0;
			} else {
				if ( autoRepeatDelay == 10 ) {
					choice--; redraw = 1;
				} else {
					autoRepeatDelay++;
				}
			}
		}
		if ( newJoy & BUTTON_DOWN && choice < numFiles-1 ) {
			if ( !(oldJoy & BUTTON_DOWN) ) {
				choice++; redraw = 1;
				autoRepeatDelay = 0;
			} else {
				if ( autoRepeatDelay == 10 ) {
					choice++; redraw = 1;
				} else {
					autoRepeatDelay++;
				}
			}
		}
		if ( newJoy & BUTTON_START ) {
			doSelect(&fs, choice);
		}
		oldJoy = newJoy;
		
		VDP_waitVSync();
		if ( redraw ) {
			VDP_clearPlan(VDP_PLAN_A, 1);
			VDP_waitDMACompletion();
			for ( i = 2; i < 26; i++ ) {
				if ( i >= 11-choice && i < numFiles-choice+11 ) {
					str = dirList[choice+i-11]->fileName;
					if ( i == 11 ) {
						VDP_setTextPalette(PAL1);
						VDP_drawText(str, 2, i);
						VDP_setTextPalette(PAL0);
					} else {
						VDP_drawText(str, 2, i);
					}
				}
			}
		}
		redraw = 0;
	}
}
