#include <libfpgalink.h>
#include <liberror.h>
#include "range.h"
#include "mem.h"

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

int umdkDirectWriteBytes(
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

int umdkDirectWriteWord(
	struct FLContext *handle, const uint32 address, uint16 value, const char **error)
{
	int retVal = 0;
	uint8 buf[2];
	int status;
	buf[1] = (uint8)value;
	value >>= 8;
	buf[0] = (uint8)value;
	status = umdkDirectWriteBytes(handle, address, 2, buf, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

int umdkDirectWriteLong(
	struct FLContext *handle, const uint32 address, uint32 value, const char **error)
{
	int retVal = 0;
	uint8 buf[4];
	int status;
	buf[3] = (uint8)value;
	value >>= 8;
	buf[2] = (uint8)value;
	value >>= 8;
	buf[1] = (uint8)value;
	value >>= 8;
	buf[0] = (uint8)value;
	status = umdkDirectWriteBytes(handle, address, 4, buf, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

int umdkDirectReadBytes(
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

int umdkDirectReadWord(
	struct FLContext *handle, const uint32 address, uint16 *const pValue, const char **error)
{
	int retVal = 0;
	uint8 buf[2];
	uint16 value;
	int status = umdkDirectReadBytes(handle, address, 2, buf, error);
	CHECK_STATUS(status, status, cleanup);
	value = buf[0];
	value <<= 8;
	value |= buf[1];
	*pValue = value;
cleanup:
	return retVal;
}

int umdkDirectReadLong(
	struct FLContext *handle, const uint32 address, uint32 *const pValue, const char **error)
{
	int retVal = 0;
	uint8 buf[4];
	uint32 value;
	int status = umdkDirectReadBytes(handle, address, 4, buf, error);
	CHECK_STATUS(status, status, cleanup);
	value = buf[0];
	value <<= 8;
	value |= buf[1];
	value <<= 8;
	value |= buf[2];
	value <<= 8;
	value |= buf[3];
	*pValue = value;
cleanup:
	return retVal;
}

int umdkExecuteCommand(
	struct FLContext *handle, uint16 command, uint32 address, uint32 length,
	const uint8 *sendData, uint8 *recvData, const char **error)
{
	int retVal = 0;
	int status;
	uint16 cmdFlag;

	// Send the request data, if necessary
	if ( sendData ) {
		status = umdkDirectWriteBytes(handle, 0x400454, length, sendData, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Setup the parameter block
	status = umdkDirectWriteWord(handle, 0x400402, command, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, 0x400404, address, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, 0x400408, length, error);
	CHECK_STATUS(status, status, cleanup);

	// Start the command executing
	status = umdkDirectWriteWord(handle, 0x400400, 0x0002, error); // cmdFlag (2 = EXECUTE)
	CHECK_STATUS(status, status, cleanup);
	
	// Wait for execution to complete
	do {
		status = umdkDirectReadWord(handle, 0x400400, &cmdFlag, NULL);
		CHECK_STATUS(status, status, cleanup);
	} while ( cmdFlag == 0x0002 );

	// Get the response data, if necessary
	if ( recvData ) {
		status = umdkDirectReadBytes(handle, 0x400454, length, recvData, error);
		CHECK_STATUS(status, status, cleanup);
	}
cleanup:
	return retVal;
}

int umdkRemoteAcquire(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status;
	uint32 vbAddr;
	uint16 oldOp, cmdFlag;
	union RegUnion {
		struct Registers reg;
		uint32 longs[18];
		uint8 bytes[18*4];
	} *const u = (union RegUnion *)regs;
	int i;

	// See if the monitor is already running
	status = umdkDirectReadWord(handle, 0x400400, &cmdFlag, error);
	CHECK_STATUS(status, status, cleanup);

	// If monitor is already running, we've got nothing to do. Otherwise...
	if ( cmdFlag != 0x0001 ) {
		// Read address of VDP vertical interrupt vector
		status = umdkDirectReadLong(handle, 0x78, &vbAddr, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Read opcode at vbAddr
		status = umdkDirectReadWord(handle, vbAddr, &oldOp, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Replace opcode at vbAddr with illegal instruction, causing MD to enter monitor
		status = umdkDirectWriteWord(handle, vbAddr, 0x4AFC, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Wait for monitor to start
		do {
			status = umdkDirectReadWord(handle, 0x400400, &cmdFlag, error);
			CHECK_STATUS(status, status, cleanup);
		} while ( cmdFlag != 0x0001 );
		
		// Write old opcode back at vbAddr
		status = umdkDirectWriteWord(handle, vbAddr, oldOp, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Read saved registers
	status = umdkDirectReadBytes(handle, 0x40040C, 18*4, u->bytes, error);
	CHECK_STATUS(status, status, cleanup);
	for ( i = 0; i < 18; i++ ) {
		u->longs[i] = bigEndian32(u->longs[i]);
	}
cleanup:
	return retVal;
}
