#include <stdlib.h>
#include <string.h>
#include <libfpgalink.h>
#include <liberror.h>
#include "range.h"
#include "mem.h"

static void prepMemCtrlCmd(uint8 cmd, uint32 addr, uint8 *buf) {
	buf[0] = cmd;
	buf[3] = (uint8)addr;
	addr >>= 8;
	buf[2] = (uint8)addr;
	addr >>= 8;
	buf[1] = (uint8)addr;
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
	if ( isInside(MONITOR, 0x80000, address, byteCount) ) {
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
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectWriteFile(): Address must be even!");
	CHECK_STATUS(byteCount&1, 3, cleanup, "umdkDirectWriteFile(): File must have even length!");

	// Get word-count and word-address
	wordAddr = address / 2;
	wordCount = byteCount / 2;

	// Prepare the write command
	prepMemCtrlCmd(0x00, wordAddr, command);
	prepMemCtrlCmd(0x80, wordCount, command+4);

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
	if ( isInside(MONITOR, 0x80000, address, count) ) {
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
			"umdkDirectWriteBytes(): Illegal direct-write to 0x%06X-0x%06X range!",
			address, address+count-1
		);
	}

	// Next verify that the write is to an even address, and has even length
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectWriteBytes(): Address must be even!");
	CHECK_STATUS(count&1, 3, cleanup, "umdkDirectWriteBytes(): Count must be even!");

	// Get word-count and word-addres
	wordAddr = address / 2;
	wordCount = count / 2;

	// Prepare the write command
	prepMemCtrlCmd(0x00, wordAddr, command);
	prepMemCtrlCmd(0x80, wordCount, command+4);

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
	uint8 *tmpBuf = NULL;

	// First verify the read is in a legal range
	if ( isInside(MONITOR, 0x80000, address, count) ) {
		// Read is from the UMDKv2-reserved 512KiB of address-space at 0x400000. The mapping for this
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
			"umdkDirectReadBytes(): Illegal direct-read from 0x%06X-0x%06X range!",
			address, address+count-1
		);
	}

	// Reads from odd addresses or for odd lengths need to be done via a temporary buffer
	if ( address & 1 ) {
		// Odd address
		tmpBuf = (uint8*)malloc(count);
		CHECK_STATUS(!tmpBuf, 2, cleanup, "umdkDirectReadBytes(): Allocation error!");
		wordAddr = address / 2;
		wordCount = 1 + count / 2;
		
		// Prepare the read command
		prepMemCtrlCmd(0x00, wordAddr, command);
		prepMemCtrlCmd(0x40, wordCount, command+4);
		
		// Send the read request
		status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
		CHECK_STATUS(status, 3, cleanup);
		
		// Receive the data
		status = flReadChannel(handle, 0x00, 2*wordCount, tmpBuf, NULL);
		CHECK_STATUS(status, 4, cleanup);
		memcpy(data, tmpBuf+1, count);
	} else {
		// Even address
		if ( count & 1 ) {
			// Even address, odd count
			tmpBuf = (uint8*)malloc(count);
			CHECK_STATUS(!tmpBuf, 5, cleanup, "umdkDirectReadBytes(): Allocation error!");
			wordAddr = address / 2;
			wordCount = 1 + count / 2;
			
			// Prepare the read command
			prepMemCtrlCmd(0x00, wordAddr, command);
			prepMemCtrlCmd(0x40, wordCount, command+4);
			
			// Send the read request
			status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
			CHECK_STATUS(status, 6, cleanup);
			
			// Receive the data
			status = flReadChannel(handle, 0x00, 2*wordCount, tmpBuf, NULL);
			CHECK_STATUS(status, 7, cleanup);
			memcpy(data, tmpBuf, count);
		} else {
			// Even address, even count
			wordAddr = address / 2;
			wordCount = count / 2;
			
			// Prepare the read command
			prepMemCtrlCmd(0x00, wordAddr, command);
			prepMemCtrlCmd(0x40, wordCount, command+4);
			
			// Send the read request
			status = flWriteChannelAsync(handle, 0x00, 8, command, NULL);
			CHECK_STATUS(status, 8, cleanup);
			
			// Receive the data
			status = flReadChannel(handle, 0x00, count, data, NULL);
			CHECK_STATUS(status, 9, cleanup);
		}
	}
cleanup:
	free(tmpBuf);
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
	struct FLContext *handle, Command command, uint32 address, uint32 length,
	const uint8 *sendData, uint8 *recvData, const char **error)
{
	int retVal = 0;
	int status;
	uint16 cmdFlag;

	// Send the request data, if necessary
	if ( sendData ) {
		status = umdkDirectWriteBytes(handle, CMD_MEM, length, sendData, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Setup the parameter block
	status = umdkDirectWriteWord(handle, CMD_INDEX, command, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, CMD_ADDR, address, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, CMD_LEN, length, error);
	CHECK_STATUS(status, status, cleanup);

	// Start the command executing
	status = umdkDirectWriteWord(handle, CMD_FLAG, CF_CMD, error);
	CHECK_STATUS(status, status, cleanup);
	
	// Wait for execution to complete
	do {
		status = umdkDirectReadWord(handle, CMD_FLAG, &cmdFlag, NULL);
		CHECK_STATUS(status, status, cleanup);
	} while ( cmdFlag == CF_CMD );

	// Get the response data, if necessary
	if ( recvData ) {
		status = umdkDirectReadBytes(handle, CMD_MEM, length, recvData, error);
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
	uint32 vbAddr, oldIL;
	uint16 oldOp, cmdFlag;
	union RegUnion {
		struct Registers reg;
		uint32 longs[18];
		uint8 bytes[18*4];
	} *const u = (union RegUnion *)regs;
	int i;

	// See if the monitor is already running
	status = umdkDirectReadWord(handle, CMD_FLAG, &cmdFlag, error);
	CHECK_STATUS(status, status, cleanup);

	// If monitor is already running, we've got nothing to do. Otherwise...
	if ( cmdFlag != CF_READY ) {
		// Read address of VDP vertical interrupt vector
		status = umdkDirectReadLong(handle, VB_VEC, &vbAddr, error);
		CHECK_STATUS(status, status, cleanup);

		// Read illegal instruction vector
		status = umdkDirectReadLong(handle, IL_VEC, &oldIL, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Read opcode at vbAddr
		status = umdkDirectReadWord(handle, vbAddr, &oldOp, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Write monitor address to illegal instruction vector
		status = umdkDirectWriteLong(handle, IL_VEC, MONITOR, error);
		CHECK_STATUS(status, status, cleanup);

		// Replace opcode at vbAddr with illegal instruction, causing MD to enter monitor
		status = umdkDirectWriteWord(handle, vbAddr, ILLEGAL, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Wait for monitor to start
		do {
			status = umdkDirectReadWord(handle, CMD_FLAG, &cmdFlag, error);
			CHECK_STATUS(status, status, cleanup);
		} while ( cmdFlag != CF_READY );
		
		// Restore old opcode back at vbAddr
		status = umdkDirectWriteWord(handle, vbAddr, oldOp, error);
		CHECK_STATUS(status, status, cleanup);

		// Restore illegal instruction vector
		status = umdkDirectWriteLong(handle, IL_VEC, oldIL, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Read saved registers
	status = umdkDirectReadBytes(handle, CMD_REGS, 18*4, u->bytes, error);
	CHECK_STATUS(status, status, cleanup);
	for ( i = 0; i < 18; i++ ) {
		u->longs[i] = bigEndian32(u->longs[i]);
	}
cleanup:
	return retVal;
}

int umdkIndirectWriteBytes(
	struct FLContext *handle, uint32 address, const uint32 count, const uint8 *const data,
	const char **error)
{
	int retVal = 0;
	int status;

	// Verify that the write is to an even address, and has even length
	CHECK_STATUS(address&1, 2, cleanup, "umdkIndirectWriteBytes(): Address must be even!");
	CHECK_STATUS(count&1, 3, cleanup, "umdkIndirectWriteBytes(): Count must be even!");

	// Execute the write
	status = umdkExecuteCommand(handle, CMD_WRITE, address, count, data, NULL, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

int umdkIndirectReadBytes(
	struct FLContext *handle, uint32 address, const uint32 count, uint8 *const data,
	const char **error)
{
	int retVal = 0;
	int status;
	uint32 wordCount;
	uint8 *tmpBuf = NULL;

	// Reads from odd addresses or for odd lengths need to be done via a temporary buffer
	if ( address & 1 ) {
		// Odd address
		tmpBuf = (uint8*)malloc(count);
		CHECK_STATUS(!tmpBuf, 2, cleanup, "umdkIndirectReadBytes(): Allocation error!");
		wordCount = 1 + count / 2;

		// Execute the read
		status = umdkExecuteCommand(handle, CMD_READ, address-1, 2*wordCount, NULL, tmpBuf, error);
		CHECK_STATUS(status, status, cleanup);
		memcpy(data, tmpBuf+1, count);
	} else {
		// Even address
		if ( count & 1 ) {
			// Even address, odd count
			tmpBuf = (uint8*)malloc(count);
			CHECK_STATUS(!tmpBuf, 5, cleanup, "umdkDirectReadBytes(): Allocation error!");
			wordCount = 1 + count / 2;

			// Execute the read
			status = umdkExecuteCommand(handle, CMD_READ, address, 2*wordCount, NULL, tmpBuf, error);
			CHECK_STATUS(status, status, cleanup);
			memcpy(data, tmpBuf, count);
		} else {
			// Even address, even count
			status = umdkExecuteCommand(handle, CMD_READ, address, count, NULL, data, error);
			CHECK_STATUS(status, status, cleanup);
		}
	}
cleanup:
	free(tmpBuf);
	return retVal;
}

int umdkStep(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status;
	union RegUnion {
		struct Registers reg;
		uint32 longs[18];
		uint8 bytes[18*4];
	} *const u = (union RegUnion *)regs;
	uint32 oldTR;
	int i;

	// Read trace vector
	status = umdkDirectReadLong(handle, TR_VEC, &oldTR, error);
	CHECK_STATUS(status, status, cleanup);

	// Write monitor address to trace vector
	status = umdkDirectWriteLong(handle, TR_VEC, MONITOR, error);
	CHECK_STATUS(status, status, cleanup);

	// Execute step
	status = umdkExecuteCommand(handle, CMD_STEP, 0, 0, NULL, NULL, error);
	CHECK_STATUS(status, status, cleanup);

	// Restore old trace vector
	status = umdkDirectWriteLong(handle, TR_VEC, oldTR, error);
	CHECK_STATUS(status, status, cleanup);

	// Read saved registers
	status = umdkDirectReadBytes(handle, CMD_REGS, 18*4, u->bytes, error);
	CHECK_STATUS(status, status, cleanup);
	for ( i = 0; i < 18; i++ ) {
		u->longs[i] = bigEndian32(u->longs[i]);
	}
cleanup:
	return retVal;
}

int umdkCont(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status;
	union RegUnion {
		struct Registers reg;
		uint32 longs[18];
		uint8 bytes[18*4];
	} *const u = (union RegUnion *)regs;
	int i;
	uint8 byte;

	// Find out whether we're in reset or not
	status = flReadChannel(handle, 0x01, 1, &byte, error);
	CHECK_STATUS(status, 9, cleanup);
	if ( byte & 1 ) {
		// MD in reset - start it running
		byte &= 0xFE;
		status = flWriteChannelAsync(handle, 0x01, 1, &byte, error);
		CHECK_STATUS(status, 9, cleanup);
	} else {
		// MD not in reset
		status = umdkRemoteAcquire(handle, regs, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Execute cont
		status = umdkExecuteCommand(handle, CMD_CONT, 0, 0, NULL, NULL, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Acquire monitor
	status = umdkRemoteAcquire(handle, regs, error);
	CHECK_STATUS(status, status, cleanup);

	// Read saved registers
	status = umdkDirectReadBytes(handle, CMD_REGS, 18*4, u->bytes, error);
	CHECK_STATUS(status, status, cleanup);
	for ( i = 0; i < 18; i++ ) {
		u->longs[i] = bigEndian32(u->longs[i]);
	}
cleanup:
	return retVal;
}

int umdkWriteBytes(
	struct FLContext *handle, uint32 address, const uint32 count, const uint8 *const data,
	const char **error)
{
	// Determine from the range whether to use a direct or indirect write
	if ( isInside(MONITOR, 0x80000, address, count) || isInside(0, 0x80000, address, count) ) {
		return umdkDirectWriteBytes(handle, address, count, data, error);
	} else {
		return umdkIndirectWriteBytes(handle, address, count, data, error);
	}
}

int umdkReadBytes(
	struct FLContext *handle, uint32 address, const uint32 count, uint8 *const data,
	const char **error)
{
	// Determine from the range whether to use a direct or indirect read
	if ( isInside(MONITOR, 0x80000, address, count) || isInside(0, 0x80000, address, count) ) {
		return umdkDirectReadBytes(handle, address, count, data, error);
	} else {
		return umdkIndirectReadBytes(handle, address, count, data, error);
	}
}

int umdkSetRegister(struct FLContext *handle, Register reg, uint32 value, const char **error) {
	int retVal = 0;
	int status = umdkDirectWriteLong(handle, CMD_REGS+4*reg, value, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

int umdkGetRegister(struct FLContext *handle, Register reg, uint32 *value, const char **error) {
	int retVal = 0;
	int status = umdkDirectReadLong(handle, CMD_REGS+4*reg, value, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}
