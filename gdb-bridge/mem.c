#include <libfpgalink.h>
#include <liberror.h>
#include "range.h"

int umdkDirectWriteData(
	struct FLContext *handle, uint32 address, const uint32 count, const uint8 *const data,
	const char **error)
{
	int retVal = 0;
	FLStatus status;
	uint8 command[8];
	uint32 wordAddr;
	uint32 wordCount;

	// First verify the write is in a legal range
	//
	if ( isInside(0x400000, 0x80000, address, count) ) {
		// Write is to the UMDKv2-reserved 512KiB of address-space at 0x400000. The mapping for this
		// is fixed to the top 512KiB of SDRAM, so we need to transpose the MD virtual address to
		// get the correct SDRAM physical address.
		address += 0xb80000;
	} else if ( !isInside(0, 0x80000, address, count) ) {
		// The only other region of the address-space which is directly host-addressable is the
		// bottom 512KiB of address-space, which has a logical-physical mapping that is guaranteed
		// by SEGA to be 1:1 (i.e no transpose necessary). So any attempt to directly access memory
		// anywhere else is an error.
		CHECK_STATUS(
			true, 1, cleanup,
			"umdkDirectWriteData(): Illegal direct-write to 0x%06X-0x%06X range!",
			address, address+count-1
		);
	}

	// Next verify that the write is to an even address, and has even countgth
	//
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectWriteData(): Address must be even!");
	CHECK_STATUS(count&1, 3, cleanup, "umdkDirectWriteData(): Count must be even!");

	// Get word-count and word-addres
	wordAddr = address / 2;
	wordCount = count / 2;

	// Prepare the write command
	command[0] = 0x00; // set mem-pipe pointer
	command[3] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[2] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[1] = (uint8)wordAddr;
	
	command[4] = 0x80; // write words to SDRAM
	command[7] = (uint8)wordCount;
	wordCount >>= 8;
	command[6] = (uint8)wordCount;
	wordCount >>= 8;
	command[5] = (uint8)wordCount;

	// Do the write
	status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
	CHECK_STATUS(status, 4, cleanup);
	status = flWriteChannelAsync(handle, 0x00, count, data, NULL);
	CHECK_STATUS(status, 5, cleanup);
cleanup:
	return retVal;
}

int umdkDirectWriteFile(
	struct FLContext *handle, uint32 address, const char *fileName,
	const char **error)
{
	int retVal = 0;
	FLStatus status;
	uint8 command[8];
	uint32 wordAddr;
	size_t byteCount;
	size_t wordCount;
	uint8 *const fileData = flLoadFile(fileName, &byteCount);
	CHECK_STATUS(!fileData, 1, cleanup, "umdkDirectWriteFile(): Cannot read from %s!", fileName);

	// Verify the write is in a legal range
	//
	if ( isInside(0x400000, 0x80000, address, byteCount) ) {
		// Write is to the UMDKv2-reserved 512KiB of address-space at 0x400000. The mapping for this
		// is fixed to the top 512KiB of SDRAM, so we need to transpose the MD virtual address to
		// get the correct SDRAM physical address.
		address += 0xb80000;
	} else if ( !isInside(0, 0x80000, address, byteCount) ) {
		// The only other region of the address-space which is directly host-addressable is the
		// bottom 512KiB of address-space, which has a logical-physical mapping that is guaranteed
		// by SEGA to be 1:1 (i.e no transpose necessary). So any attempt to directly access memory
		// anywhere else is an error.
		CHECK_STATUS(
			true, 1, cleanup,
			"umdkDirectWriteFile(): Illegal direct-write to 0x%06X-0x%06X range!",
			address, address+byteCount-1
		);
	}

	// Next verify that the write is to an even address, and has even length
	//
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectWriteFile(): Address must be even!");
	CHECK_STATUS(byteCount&1, 3, cleanup, "umdkDirectWriteFile(): File must have even length!");

	// Get word-count and word-address
	wordAddr = address / 2;
	wordCount = byteCount / 2;

	// Prepare the write command
	command[0] = 0x00; // set mem-pipe pointer
	command[3] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[2] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[1] = (uint8)wordAddr;
	
	command[4] = 0x80; // write words to SDRAM
	command[7] = (uint8)wordCount;
	wordCount >>= 8;
	command[6] = (uint8)wordCount;
	wordCount >>= 8;
	command[5] = (uint8)wordCount;

	// Do the write
	status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
	CHECK_STATUS(status, 4, cleanup);
	status = flWriteChannelAsync(handle, 0x00, byteCount, fileData, NULL);
	CHECK_STATUS(status, 5, cleanup);
cleanup:
	flFreeFile(fileData);
	return retVal;
}

int umdkDirectRead(
	struct FLContext *handle, uint32 address, const uint32 count, uint8 *const data,
	const char **error)
{
	int retVal = 0;
	FLStatus status;
	uint8 command[8];
	uint32 wordAddr;
	uint32 wordCount;

	// First verify the write is in a legal range
	//
	if ( isInside(0x400000, 0x80000, address, count) ) {
		// Write is to the UMDKv2-reserved 512KiB of address-space at 0x400000. The mapping for this
		// is fixed to the top 512KiB of SDRAM, so we need to transpose the MD virtual address to
		// get the correct SDRAM physical address.
		address += 0xb80000;
	} else if ( !isInside(0, 0x80000, address, count) ) {
		// The only other region of the address-space which is directly host-addressable is the
		// bottom 512KiB of address-space, which has a logical-physical mapping that is guaranteed
		// by SEGA to be 1:1 (i.e no transpose necessary). So any attempt to directly access memory
		// anywhere else is an error.
		CHECK_STATUS(
			true, 1, cleanup,
			"umdkDirectRead(): Illegal direct-read from 0x%06X-0x%06X range!",
			address, address+count-1
		);
	}

	// Next verify that the write is to an even address, and has even length
	//
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectRead(): Address must be even!");
	CHECK_STATUS(count&1, 3, cleanup, "umdkDirectRead(): Count must be even!");

	// Get word-count and word-address
	wordAddr = address / 2;
	wordCount = count / 2;

	// Prepare the write command
	command[0] = 0x00; // set mem-pipe pointer
	command[3] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[2] = (uint8)wordAddr;
	wordAddr >>= 8;
	command[1] = (uint8)wordAddr;
	
	command[4] = 0x40; // read words from SDRAM
	command[7] = (uint8)wordCount;
	wordCount >>= 8;
	command[6] = (uint8)wordCount;
	wordCount >>= 8;
	command[5] = (uint8)wordCount;

	// Send the read request
	status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
	CHECK_STATUS(status, 4, cleanup);

	// Receive the data
	status = flReadChannel(handle, 0x00, count, data, NULL);
	CHECK_STATUS(status, 5, cleanup);
cleanup:
	return retVal;
}
