#include <stdio.h>
#include <stdlib.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <liberror.h>

// Unused function to find a big-endian address
//
void findAddress(const uint8 *romFile, uint32 romLength, uint32 address) {
	uint8 addrArr[4];
	uint32 i;
	addrArr[3] = (uint8)address;
	address >>= 8;
	addrArr[2] = (uint8)address;
	address >>= 8;
	addrArr[1] = (uint8)address;
	address >>= 8;
	addrArr[0] = (uint8)address;
	for ( i = 0; i < romLength; i++ ) {
		if (
			romFile[i] == addrArr[0] &&
			romFile[i+1] == addrArr[1] &&
			romFile[i+2] == addrArr[2] &&
			romFile[i+3] == addrArr[3] )
		{
			printf("Candidate: 0x%08X\n", i);
		}
	}
}

// Find offsets where a byte changes from one specific value to another
//
int main(int argc, char *argv[]) {
	int retVal;
	uint8 *beforeFile = NULL;
	uint8 *afterFile = NULL;
	uint8 byteBefore;
	uint8 byteAfter;
	size_t beforeLength;
	size_t afterLength;
	uint32 i;
	if ( argc != 5 ) {
		fprintf(stderr, "Synopsis: %s <before.bin> <after.bin> <byteBefore> <byteAfter>\n", argv[0]);
		FAIL(1, cleanup);
	}
	beforeFile = flLoadFile(argv[1], &beforeLength);
	if ( !beforeFile ) {
		fprintf(stderr, "File %s cannot be loaded\n", argv[1]);
		FAIL(2, cleanup);
	}
	afterFile = flLoadFile(argv[2], &afterLength);
	if ( !afterFile ) {
		fprintf(stderr, "File %s cannot be loaded\n", argv[2]);
		FAIL(2, cleanup);
	}
	if ( beforeLength != afterLength ) {
		fprintf(stderr, "Files are of different lengths\n");
		FAIL(3, cleanup);
	}
	byteBefore = (uint8)strtoul(argv[3], NULL, 0);
	byteAfter = (uint8)strtoul(argv[4], NULL, 0);
	for ( i = 0; i < beforeLength; i++ ) {
		if ( beforeFile[i] == byteBefore && afterFile[i] == byteAfter ) {
			printf("Candidate: 0x%08X\n", i);
		}
	}
	retVal = 0;
cleanup:
	flFreeFile(afterFile);
	flFreeFile(beforeFile);
	return retVal;
}
