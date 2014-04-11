#include <stdlib.h>
#include <string.h>
#include <libfpgalink.h>
#include <liberror.h>
#include "range.h"
#include "mem.h"
#include "escape.h"

// Forward-declare local functions
static void prepMemCtrlCmd(uint8 cmd, uint32 addr, uint8 *buf);
static int umdkIndirectReadBytes(
	struct FLContext *handle, uint32 address, const uint32 count, uint8 *const data,
	const char **error);
static int umdkIndirectWriteBytes(
	struct FLContext *handle, uint32 address, const uint32 count, const uint8 *const data,
	const char **error);


// *************************************************************************************************
// **                                Direct read/write operations                                 **
// *************************************************************************************************

// Direct-write a binary file to the specified address. The area of memory to be written must reside
// entirely within one of the two direct-writable memory areas (0x000000-0x07FFFF and
// 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively) and must have an even start
// address and length. The MegaDrive need not be suspended at the monitor.
//
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
	status = flWriteChannelAsync(handle, 0x00, 8, command, error);
	CHECK_STATUS(status, 4, cleanup);
	status = flWriteChannelAsync(handle, 0x00, byteCount, fileData, error);
	CHECK_STATUS(status, 5, cleanup);
cleanup:
	flFreeFile(fileData);
	return retVal;
}

// Direct-write a sequence of bytes to the specified address. The area of memory to be written must
// reside entirely within one of the two direct-writable memory areas (0x000000-0x07FFFF and
// 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively) and must have an even start
// address and length. The MegaDrive need not be suspended at the monitor.
//
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
	status = flWriteChannelAsync(handle, 0x00, 8, command, error);
	CHECK_STATUS(status, 4, cleanup);
	status = flWriteChannelAsync(handle, 0x00, count, data, error);
	CHECK_STATUS(status, 5, cleanup);
cleanup:
	return retVal;
}

// Direct-write one 16-bit word to the specified address. The target word must reside entirely
// within one of the two direct-writable memory areas (0x000000-0x07FFFF and 0x400000-0x47FFFF,
// mapped to SDRAM pages 0 and 31 respectively) and must have an even start address. The MegaDrive
// need not be suspended at the monitor.
//
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

// Direct-write one 32-bit longword to the specified address. The target longword must reside
// entirely within one of the two direct-writable memory areas (0x000000-0x07FFFF and
// 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively) and must have an even start
// address. The MegaDrive need not be suspended at the monitor.
//
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

// Synchronously direct-read a sequence of bytes from the specified address. The area of memory to
// be read must reside entirely within one of the two direct-readable memory areas
// (0x000000-0x07FFFF and 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively). It need
// not have an even start address or length. The MegaDrive need not be suspended at the monitor.
//
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
		status = flWriteChannelAsync(handle, 0x00, 8, command, error);
		CHECK_STATUS(status, 3, cleanup);
		
		// Receive the data
		status = flReadChannel(handle, 0x00, 2*wordCount, tmpBuf, error);
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
			status = flWriteChannelAsync(handle, 0x00, 8, command, error);
			CHECK_STATUS(status, 6, cleanup);
			
			// Receive the data
			status = flReadChannel(handle, 0x00, 2*wordCount, tmpBuf, error);
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
			status = flWriteChannelAsync(handle, 0x00, 8, command, error);
			CHECK_STATUS(status, 8, cleanup);
			
			// Receive the data
			status = flReadChannel(handle, 0x00, count, data, error);
			CHECK_STATUS(status, 9, cleanup);
		}
	}
cleanup:
	free(tmpBuf);
	return retVal;
}

// Asynchronously direct-read a sequence of bytes from the specified address. The area of memory to
// be read must reside entirely within one of the two direct-readable memory areas
// (0x000000-0x07FFFF and 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively). Unlike
// umdkDirectReadBytes(), this function must be given an even start address and count. The
// MegaDrive need not be suspended at the monitor. This does an asynchronous read, so each call to
// this function must match a later call to FPGALink's flReadChannelAsyncAwait(), to retrieve the
// actual data.
//
int umdkDirectReadBytesAsync(
	struct FLContext *handle, uint32 address, const uint32 count, const char **error)
{
	int retVal = 0;
	FLStatus status;
	uint8 command[8];

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
			"umdkDirectReadBytesAsync(): Illegal direct-read from 0x%06X-0x%06X range!",
			address, address+count-1
		);
	}

	// Next verify that the read is to an even address, and has even length
	CHECK_STATUS(address&1, 2, cleanup, "umdkDirectReadBytesAsync(): Address must be even!");
	CHECK_STATUS(count&1, 3, cleanup, "umdkDirectReadBytesAsync(): Count must be even!");

	// Prepare the read command
	prepMemCtrlCmd(0x00, address/2, command);
	prepMemCtrlCmd(0x40, count/2, command+4);
	
	// Send the read request
	status = flWriteChannelAsync(handle, 0x00, 8, command, error);
	CHECK_STATUS(status, 8, cleanup);
	
	// Submit the read
	status = flReadChannelAsyncSubmit(handle, 0x00, count, NULL, error);
	CHECK_STATUS(status, 9, cleanup);
cleanup:
	return retVal;
}

// Direct-read one 16-bit word from the specified address. The source word must reside entirely
// within one of the two direct-readable memory areas (0x000000-0x07FFFF and 0x400000-0x47FFFF,
// mapped to SDRAM pages 0 and 31 respectively). It need not have an even start address. The
// MegaDrive need not be suspended at the monitor.
//
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

// Direct-read one 32-bit longword from the specified address. The source longword must reside
// entirely within one of the two direct-readable memory areas (0x000000-0x07FFFF and
// 0x400000-0x47FFFF, mapped to SDRAM pages 0 and 31 respectively). It need not have an even start
// address. The MegaDrive need not be suspended at the monitor.
//
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


// *************************************************************************************************
// **                                Generic read/write operations                                **
// *************************************************************************************************

// Write a sequence of bytes to the specified address. If the region to be written resides entirely
// within one of the two direct-writable memory areas (0x000000-0x07FFFF and 0x400000-0x47FFFF,
// mapped to SDRAM pages 0 and 31 respectively), then the write is done directly without the need
// for the MegaDrive to be suspended at the monitor. Otherwise, it is done through the monitor. In
// both cases, the region to be written must have an even start address and length.
//
int umdkWriteBytes(
	struct FLContext *handle, uint32 address, const uint32 count, const uint8 *const data,
	const char **error)
{
	// GDB sometimes requests zero-length writes, which succeed trivially
	if ( count == 0 ) {
		return 0;
	}

	// Determine from the range whether to use a direct or indirect write
	if ( isInside(MONITOR, 0x80000, address, count) || isInside(0, 0x80000, address, count) ) {
		return umdkDirectWriteBytes(handle, address, count, data, error);
	} else {
		return umdkIndirectWriteBytes(handle, address, count, data, error);
	}
}

int umdkWriteWord(
	struct FLContext *handle, const uint32 address, uint16 value, const char **error)
{
	int retVal = 0;
	uint8 buf[2];
	int status;
	buf[1] = (uint8)value;
	value >>= 8;
	buf[0] = (uint8)value;
	status = umdkWriteBytes(handle, address, 2, buf, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

int umdkWriteLong(
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
	status = umdkWriteBytes(handle, address, 4, buf, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// Read a sequence of bytes from the specified address. If the region to be read resides entirely
// within one of the two direct-readable memory areas (0x000000-0x07FFFF and 0x400000-0x47FFFF,
// mapped to SDRAM pages 0 and 31 respectively), then the read is done directly without the need
// for the MegaDrive to be suspended at the monitor. Otherwise, it is done through the monitor. In
// both cases, the region to be read need not have an even start address and length.
//
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

int umdkReadWord(
	struct FLContext *handle, const uint32 address, uint16 *const pValue, const char **error)
{
	int retVal = 0;
	uint8 buf[2];
	uint16 value;
	int status = umdkReadBytes(handle, address, 2, buf, error);
	CHECK_STATUS(status, status, cleanup);
	value = buf[0];
	value <<= 8;
	value |= buf[1];
	*pValue = value;
cleanup:
	return retVal;
}

int umdkReadLong(
	struct FLContext *handle, const uint32 address, uint32 *const pValue, const char **error)
{
	int retVal = 0;
	uint8 buf[4];
	uint32 value;
	int status = umdkReadBytes(handle, address, 4, buf, error);
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

// *************************************************************************************************
// **                                Low-level CPU-state operations                               **
// *************************************************************************************************

// Set the specified register to the specified value. The MegaDrive must be suspended at the
// monitor.
//
int umdkSetRegister(struct FLContext *handle, Register reg, uint32 value, const char **error) {
	int retVal = 0;
	int status = umdkDirectWriteLong(handle, CB_REGS+4*reg, value, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// Read the specified register. The MegaDrive must be suspended at the monitor.
//
int umdkGetRegister(struct FLContext *handle, Register reg, uint32 *value, const char **error) {
	int retVal = 0;
	int status = umdkDirectReadLong(handle, CB_REGS+4*reg, value, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// Continually poll the "command flag" word at 0x400400 waiting for the MD to enter the monitor.
// It will wait indefinitely for that to happen, so it's necessary to first place a breakpoint
// somewhere in the code, or you'll be waiting forever.
//
int umdkRemoteAcquire(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status, i;
	uint16 cmdFlag;
	union RegUnion {
		struct Registers reg;
		uint32 longs[18];
		uint8 bytes[18*4];
	} *const u = (union RegUnion *)regs;
	uint32 vbAddr;
	uint16 oldOp;

	// See if we're in the monitor already
	status = umdkDirectReadWord(handle, CB_FLAG, &cmdFlag, error);
	CHECK_STATUS(status, status, cleanup);
	if ( cmdFlag != CF_READY ) {
		// Read address of VDP vertical interrupt vector
		status = umdkDirectReadLong(handle, VB_VEC, &vbAddr, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Read opcode at vbAddr
		status = umdkDirectReadWord(handle, vbAddr, &oldOp, error);
		CHECK_STATUS(status, status, cleanup);
		
		// Wait for monitor
		do {
			if ( isInterrupted() ) {
				status = umdkDirectWriteWord(handle, vbAddr, ILLEGAL, error);
				CHECK_STATUS(status, status, cleanup);
			}
			status = umdkDirectReadWord(handle, CB_FLAG, &cmdFlag, error);
			CHECK_STATUS(status, status, cleanup);
		} while ( cmdFlag != CF_READY );
		
		// Restore old opcode back at vbAddr
		status = umdkDirectWriteWord(handle, vbAddr, oldOp, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Read saved registers, if necessary
	if ( regs ) {
		status = umdkDirectReadBytes(handle, CB_REGS, 18*4, u->bytes, error);
		CHECK_STATUS(status, status, cleanup);
		for ( i = 0; i < 18; i++ ) {
			u->longs[i] = bigEndian32(u->longs[i]);
		}
	}
cleanup:
	return retVal;
}

/*
// This is useful for debugging command sends
//
static
int dumpCommandBlock(struct FLContext *handle, const char **error) {
	int retVal = 0;
	int status;
	const int TMPSZ = 84 + 64;
	uint8 tmpBuf[TMPSZ];
	memset(tmpBuf, 0xCC, TMPSZ);
	status = umdkDirectReadBytes(handle, 0x400400, TMPSZ, tmpBuf, error);
	CHECK_STATUS(status, status, cleanup);
	printf("   cmdFlag: "); dumpSimple(tmpBuf, 2);
	printf("  cmdIndex: "); dumpSimple(tmpBuf+2, 2);
	printf("   address: "); dumpSimple(tmpBuf+4, 4);
	printf("    length: "); dumpSimple(tmpBuf+8, 4);
	printf("  dataRegs: "); dumpSimple(tmpBuf+12, 4*8);
	printf("  addrRegs: "); dumpSimple(tmpBuf+44, 4*8);
	printf("   srAndPC: "); dumpSimple(tmpBuf+76, 4*2);
	printf("    memory: "); dumpSimple(tmpBuf+84, 64);
cleanup:
	return retVal;
}
static const char *cmdNames[] = {
	"CMD_STEP",
	"CMD_CONT",
	"CMD_READ",
	"CMD_WRITE"
};
*/

// Issue a low-level monitor command. This is used for indirect reads & writes, continue and step
// operations. The MegaDrive must be suspended at the monitor. The sendData and recvData pointers
// may be NULL if no data exchange is necessary. On the MD side the monitor just invokes code via
// a jump-table indexed by the issued command.
//
int umdkExecuteCommand(
	struct FLContext *handle, Command command, uint32 address, uint32 length,
	const uint8 *sendData, uint8 *recvData, struct Registers *regs,
	const char **error)
{
	int retVal = 0;
	int status;
	//printf("umdkExecuteCommand(%s):\n", cmdNames[command]);

	// Send the request data, if necessary
	if ( sendData ) {
		status = umdkDirectWriteBytes(handle, CB_MEM, length, sendData, error);
		CHECK_STATUS(status, status, cleanup);
	}

	// Setup the parameter block
	status = umdkDirectWriteWord(handle, CB_INDEX, command, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, CB_ADDR, address, error);
	CHECK_STATUS(status, status, cleanup);
	status = umdkDirectWriteLong(handle, CB_LEN, length, error);
	CHECK_STATUS(status, status, cleanup);

	// Start the command executing
	status = umdkDirectWriteWord(handle, CB_FLAG, CF_CMD, error);
	CHECK_STATUS(status, status, cleanup);
	
	// Wait for execution to complete
	status = umdkRemoteAcquire(handle, regs, error);
	CHECK_STATUS(status, status, cleanup);

	// Get the response data, if necessary
	if ( recvData ) {
		status = umdkDirectReadBytes(handle, CB_MEM, length, recvData, error);
		CHECK_STATUS(status, status, cleanup);
	}

cleanup:
	return retVal;
}

// Have the monitor continue execution with the SR trace bit set. This will cause precisely one
// instruction of user code to execute and then return control to the monitor. Tracing only works if
// the code being executed is running in user mode. If you try to step through supervisor-mode code,
// the MegaDrive will just carry on executing, and never return control to the monitor. Meanwhile
// this function will be patiently waiting. And that's a long wait for a train don't come.
//
int umdkStep(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status;

	// Write monitor address to trace vector
	status = umdkDirectWriteLong(handle, TR_VEC, MONITOR, error);
	CHECK_STATUS(status, status, cleanup);

	// Execute step
	status = umdkExecuteCommand(handle, CMD_STEP, 0, 0, NULL, NULL, regs, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// Tell the monitor to continue execution with the (possibly new) register/memory context, until
// a breakpoint is hit. If there is no breakpoint in the execution-path, this function will wait
// forever.
//
int umdkCont(
	struct FLContext *handle, struct Registers *regs, const char **error)
{
	int retVal = 0;
	int status;

	// Write monitor address to illegal instruction vector
	status = umdkDirectWriteLong(handle, IL_VEC, MONITOR, error);
	CHECK_STATUS(status, status, cleanup);
		
	// Execute continue
	status = umdkExecuteCommand(handle, CMD_CONT, 0, 0, NULL, NULL, regs, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// *************************************************************************************************
// **                               Operations private to this file                               **
// *************************************************************************************************

// Prepare a low-level SDRAM-controller command. Three commands are accepted:
//   0x00 <u24> - set the SDRAM-controller read/write address register to u24
//   0x40 <u24> - read u24 16-bit words from the r/w addr reg, incrementing
//   0x80 <u24> - write u24 16-bit words to the r/w addr reg, incrementing
//
static
void prepMemCtrlCmd(uint8 cmd, uint32 addr, uint8 *buf) {
	buf[0] = cmd;
	buf[3] = (uint8)addr;
	addr >>= 8;
	buf[2] = (uint8)addr;
	addr >>= 8;
	buf[1] = (uint8)addr;
}

// Indirect-write a sequence of bytes to the specified address. The area of memory to be written may
// be anywhere in the MegaDrive's 16MiB address-space. It must have an even start-address and
// length. The MegaDrive must be suspended at the monitor.
//
static
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
	status = umdkExecuteCommand(handle, CMD_WRITE, address, count, data, NULL, NULL, error);
	CHECK_STATUS(status, status, cleanup);
cleanup:
	return retVal;
}

// Indirect-read a sequence of bytes from the specified address. The area of memory to be read may
// be anywhere in the MegaDrive's 16MiB address-space. It must have an even start-address and
// length. The MegaDrive must be suspended at the monitor.
//
static
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
		status = umdkExecuteCommand(handle, CMD_READ, address-1, 2*wordCount, NULL, tmpBuf, NULL, error);
		CHECK_STATUS(status, status, cleanup);
		memcpy(data, tmpBuf+1, count);
	} else {
		// Even address
		if ( count & 1 ) {
			// Even address, odd count
			tmpBuf = (uint8*)malloc(count);
			CHECK_STATUS(!tmpBuf, 5, cleanup, "umdkIndirectReadBytes(): Allocation error!");
			wordCount = 1 + count / 2;

			// Execute the read
			status = umdkExecuteCommand(handle, CMD_READ, address, 2*wordCount, NULL, tmpBuf, NULL, error);
			CHECK_STATUS(status, status, cleanup);
			memcpy(data, tmpBuf, count);
		} else {
			// Even address, even count
			status = umdkExecuteCommand(handle, CMD_READ, address, count, NULL, data, NULL, error);
			CHECK_STATUS(status, status, cleanup);
		}
	}
cleanup:
	free(tmpBuf);
	return retVal;
}


/*
  CODE TO INTERRUPT EXECUTION (USER PRESSES ESCAPE IN GDB)

	// See if the monitor is already running
	status = umdkDirectReadWord(handle, CB_FLAG, &cmdFlag, error);
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
			status = umdkDirectReadWord(handle, CB_FLAG, &cmdFlag, error);
			CHECK_STATUS(status, status, cleanup);
		} while ( cmdFlag != CF_READY );
		
		// Restore old opcode back at vbAddr
		status = umdkDirectWriteWord(handle, vbAddr, oldOp, error);
		CHECK_STATUS(status, status, cleanup);

		// Restore illegal instruction vector
		status = umdkDirectWriteLong(handle, IL_VEC, oldIL, error);
		CHECK_STATUS(status, status, cleanup);
	}
*/
