/* 
 * Copyright (C) 2011 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <liberror.h>
#include "umdk.h"
#include "i68k.h"
#include "range.h"

// Monitor and command block:
#define MONITOR_BASE 0x400000
#define CMDBLK_BASE  0x400400
#define CMDBLK_SIZE  (4*(3+18))

// Vectors to point at monitor code:
#define ILLEGAL_VECTOR 0x10
#define TRACE_VECTOR   0x24

// UMDK commands:
#define CMD_NOP   0
#define CMD_STEP  1
#define CMD_CONT  2
#define CMD_READ  3
#define CMD_WRITE 4

// FPGA Registers
#define DATA_REG   0x00
#define ADDR0_REG  0x01
#define ADDR1_REG  0x02
#define ADDR2_REG  0x03
#define FLAGS_REG  0x04
#define BRK0_REG   0x05
#define BRK1_REG   0x06
#define BRK2_REG   0x07

// FPGA flags register bits:
#define FLAG_RUN   (1<<0)
#define FLAG_HREQ  (1<<1)
#define FLAG_HACK  (1<<2)
#define FLAG_BKEN  (1<<7)

// The monitor object code (assembled from 68000 source)
extern const unsigned int monitorCodeSize;
extern const unsigned char monitorCodeData[];

// Big-endian address of monitor code:
static const uint8 monitorAddress[] = {
	0x00,
	((MONITOR_BASE >> 16) & 0xFF),
	((MONITOR_BASE >> 8) & 0xFF),
	(MONITOR_BASE & 0xFF)
};

// 68000 status register:
typedef struct {
	unsigned carry      : 1;
	unsigned overflow   : 1;
	unsigned zero       : 1;
	unsigned negative   : 1;
	unsigned extend     : 1;
	unsigned pad0       : 3;
	unsigned intMask    : 3;
	unsigned pad1       : 2;
	unsigned supervisor : 1;
	unsigned pad2       : 1;
	unsigned trace      : 1;
	unsigned pad3       : 16;
} StatusRegister;

// 68000 registers:
typedef union {
	uint32 value[18];
	struct {
		uint32 d0;
		uint32 d1;
		uint32 d2;
		uint32 d3;
		uint32 d4;
		uint32 d5;
		uint32 d6;
		uint32 d7;
		
		uint32 a0;
		uint32 a1;
		uint32 a2;
		uint32 a3;
		uint32 a4;
		uint32 a5;
		uint32 a6;
		uint32 a7;
		
		StatusRegister sr;
		uint32 pc;
	} regs;
} MachineState;

// The UMDK implementation of the I68K interface:
struct UMDK {
	I68K_MEMBERS;
	struct FLContext *fpga;
	MachineState machineState;
	uint32 brkAddress;
	uint8 flagByte;
};

// Return codes:
typedef enum {
	UMDK_SUCCESS = 0,
	UMDK_FPGALINK_ERR,
	UMDK_BUS_ERR,
	UMDK_TIMEOUT_ERR,
	UMDK_ALIGN_ERR,
	UMDK_OPEN_ERR
} UMDKStatus;

// Read a uint32 from the big-endian memory pointed to by p:
//
static uint32 readLong(const uint8 *p) {
	uint32 value = *p++;
	value <<= 8;
    value |= *p++;
    value <<= 8;
    value |= *p++;
    value <<= 8;
    value |= *p;
	return value;
}

// Write a big-endian uint32 to the memory pointed to by p:
//
static void writeLong(uint32 value, uint8 *p) {
	p[3] = value & 0x000000FF;
	value >>= 8;
	p[2] = value & 0x000000FF;
	value >>= 8;
	p[1] = value & 0x000000FF;
	value >>= 8;
	p[0] = value & 0x000000FF;
}

static UMDKStatus setBreakpointAddress(
	struct FLContext *handle, uint32 address, bool enabled, const char **error)
{
	UMDKStatus retVal;
	FLStatus fStatus;
	uint8 byte = 0x00;
	flCleanWriteBuffer(handle);

	// First disable breakpoint to stop mid-update hits:
	fStatus = flAppendWriteChannelCommand(handle, BRK0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointAddress()");

	// Now update the breakpoint address, LSB first:
	address >>= 1;
	byte = address & 0x000000FF;
	fStatus = flAppendWriteChannelCommand(handle, BRK2_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointAddress()");
	address >>= 8;
	byte = address & 0x000000FF;
	fStatus = flAppendWriteChannelCommand(handle, BRK1_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointAddress()");
	address >>= 8;
	if ( enabled ) {
		byte = (uint8)(address | FLAG_BKEN);
	} else {
		byte = (uint8)(address & 0x7F);
	}
	fStatus = flAppendWriteChannelCommand(handle, BRK0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointAddress()");

	// Play those writes now:
	fStatus = flPlayWriteBuffer(handle, 100, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointAddress()");
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

/*
static UMDKStatus getBreakpointAddress(
	struct FLContext *handle, uint32 *address, const char **error)
{
	UMDKStatus returnCode;
	FLStatus fStatus;
	uint8 byte;
	uint32 thisAddr;
	fStatus = flReadChannel(handle, 100, BRK0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, "umdkGetBreakpointAddress()", UMDK_FPGALINK_ERR);
	byte &= ~FLAG_BKEN;
	thisAddr = byte;
	thisAddr <<= 8;
	fStatus = flReadChannel(handle, 100, BRK1_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, "umdkGetBreakpointAddress()", UMDK_FPGALINK_ERR);
	thisAddr |= byte;
	thisAddr <<= 8;
	fStatus = flReadChannel(handle, 100, BRK2_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, "umdkGetBreakpointAddress()", UMDK_FPGALINK_ERR);
	thisAddr |= byte;
	thisAddr <<= 1;
	*address = thisAddr;
	return UMDK_SUCCESS;
cleanup:
	return returnCode;
}
*/

static UMDKStatus setBreakpointEnabled(
	struct FLContext *handle, bool enabledFlag, const char **error)
{
	UMDKStatus retVal;
	FLStatus fStatus;
	uint8 byte;
	uint8 oldByte;

	// Read it...
	fStatus = flReadChannel(handle, 100, BRK0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointEnabled()");
	oldByte = byte;

	// Update it...
	if ( enabledFlag ) {
		byte = (uint8)(byte | FLAG_BKEN);
	} else {
		byte = (uint8)(byte & ~FLAG_BKEN);
	}

	// Write it back
	if ( byte != oldByte ) {
		fStatus = flWriteChannel(handle, 100, BRK0_REG, 1, &byte, error);
		CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "umdkSetBreakpointEnabled()");
	}
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

/*
static UMDKStatus getBreakpointEnabled(
	struct FLContext *handle, bool *enabledFlag, const char **error)
{
	UMDKStatus returnCode;
	FLStatus fStatus;
	uint8 byte;
	fStatus = flReadChannel(handle, 100, BRK0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, "umdkGetBreakpointEnabled()", UMDK_FPGALINK_ERR);
	if ( byte & FLAG_BKEN ) {
		*enabledFlag = true;
	} else {
		*enabledFlag = false;
	}
	return UMDK_SUCCESS;
cleanup:
	return returnCode;
}
*/

// Set the HREQ flag and wait (indefinitely) for the HACK signal.
// TODO: Accept ctrl-C from GDB
//
static UMDKStatus remoteAcquire(struct UMDK *handle, bool isInterrupted, const char **error) {
	UMDKStatus retVal, uStatus;
	FLStatus fStatus;
	handle->flagByte |= FLAG_HREQ;
	fStatus = flWriteChannel(handle->fpga, 100, FLAGS_REG, 1, &handle->flagByte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "remoteAcquire()");
	fStatus = flReadChannel(handle->fpga, 100, FLAGS_REG, 1, &handle->flagByte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "remoteAcquire()");
	//printf("Attempting to acquire MegaDrive.");
	//fflush(stdout);
	while ( !(handle->flagByte & FLAG_HACK) && !isInterrupted ) {
		flSleep(100);
		//printf(".");
		//fflush(stdout);
		fStatus = flReadChannel(handle->fpga, 100, FLAGS_REG, 1, &handle->flagByte, error);
		CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "remoteAcquire()");
		isInterrupted = handle->isInterrupted((const struct I68K *)handle);
	}
	if ( isInterrupted ) {
		//printf("\nAttempting to interrupt MegaDrive.");
		//fflush(stdout);
		uStatus = setBreakpointEnabled(handle->fpga, true, error );
		CHECK_STATUS(uStatus, uStatus, cleanup, "remoteAcquire()");
		while ( !(handle->flagByte & FLAG_HACK) ) {
			flSleep(100);
			//printf(".");
			//fflush(stdout);
			fStatus = flReadChannel(handle->fpga, 100, FLAGS_REG, 1, &handle->flagByte, error);
			CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "remoteAcquire()");
		}
		uStatus = setBreakpointEnabled(handle->fpga, false, error );
		CHECK_STATUS(uStatus, uStatus, cleanup, "remoteAcquire()");
	}
	//printf("\n");
	//fflush(stdout);
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

// Prepare a UMDK command block using the cmd, param1 & param2 uint32s
//
static void prepareCommand(uint8 *cmdBlock, uint32 cmd, uint32 param1, uint32 param2) {
	writeLong(cmd, cmdBlock);
	cmdBlock += 4;
	writeLong(param1, cmdBlock);
	cmdBlock += 4;
	writeLong(param2, cmdBlock);
}

// Write the 68000 registers into the UMDK command block prior to execution.
//
static void prepareRegisters(uint8 *cmdBlock, const MachineState *state) {
	int i;
	cmdBlock += 3*4;
	for ( i = 0; i < 18; i++ ) {
		writeLong(state->value[i], cmdBlock);
		cmdBlock += 4;
	}
}

// Following execution of a UMDK command, retrieve the 68000 register values.
//
static void retrieveRegisters(MachineState *state, const uint8 *cmdBlock) {
	int i;
	uint32 *p = state->value;
	cmdBlock += 3*4;
	for ( i = 0; i < 18; i++ ) {
		*p++ = readLong(cmdBlock);
		cmdBlock += 4;
	}
}

// Fails: UMDK_FPGALINK_ERR
static UMDKStatus setAddress(
	struct FLContext *handle, uint32 address, const char **error)
{
	UMDKStatus retVal;
	FLStatus fStatus;
	uint8 byte;
	flCleanWriteBuffer(handle);

	// Write LSB:
	address >>= 1;  // get word address
	byte = address & 0x000000FF;
	fStatus = flAppendWriteChannelCommand(handle, ADDR2_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "setAddress()");

	// Write middle byte:
	address >>= 8;
	byte = address & 0x000000FF;
	fStatus = flAppendWriteChannelCommand(handle, ADDR1_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "setAddress()");

	// Write MSB
	address >>= 8;
	byte = address & 0x000000FF;
	fStatus = flAppendWriteChannelCommand(handle, ADDR0_REG, 1, &byte, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "setAddress()");

	// Play those writes now:
	fStatus = flPlayWriteBuffer(handle, 100, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "setAddress()");
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

// Fails: UMDK_ALIGN_ERR, UMDK_FPGALINK_ERR, UMDK_BUS_ERR
static UMDKStatus writeMemDirect(
	struct FLContext *handle, uint32 address, uint32 count, const uint8 *data, const char **error)
{
	UMDKStatus uStatus, retVal;
	FLStatus fStatus;
	uint8 flags;

	// Make sure it's an even number of bytes:
	CHECK_STATUS(count & 1, UMDK_ALIGN_ERR, cleanup, "writeMemDirect(): Cannot write an odd number of bytes!");

	// Get the flags register:
	fStatus = flReadChannel(handle, 100, FLAGS_REG, 1, &flags, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "writeMemDirect()");

	// Verify that the MD is in reset or has relinquished the bus
	CHECK_STATUS(
		(flags & FLAG_RUN) && !(flags & FLAG_HACK),
		UMDK_BUS_ERR, cleanup, "writeMemDirect(): Bus not under host control!");

	// Do the write
	uStatus = setAddress(handle, address, error);
	CHECK_STATUS(uStatus, uStatus, cleanup, "writeMemDirect()");
	fStatus = flWriteChannel(handle, 30000, DATA_REG, count, data, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "writeMemDirect()");
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

static UMDKStatus readMemDirect(
	struct FLContext *handle, uint32 address, uint32 count, uint8 *buf, const char **error)
{
	UMDKStatus uStatus, retVal;
	FLStatus fStatus;
	uint8 flags;

	// Make sure it's an even number of bytes:
	CHECK_STATUS(count & 1, UMDK_ALIGN_ERR, cleanup, "readMemDirect(): Cannot read an odd number of bytes!");

	// Get the flags register:
	fStatus = flReadChannel(handle, 100, FLAGS_REG, 1, &flags, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "readMemDirect()");

	// Verify that the MD is in reset or has relinquished the bus
	CHECK_STATUS(
		(flags & FLAG_RUN) && !(flags & FLAG_HACK),
		UMDK_BUS_ERR, cleanup, "readMemDirect(): Bus not under host control!");

	// Do the read
	uStatus = setAddress(handle, address, error);
	CHECK_STATUS(uStatus, uStatus, cleanup, "readMemDirect()");
	fStatus = flReadChannel(handle, 30000, DATA_REG, count, buf, error);
	CHECK_STATUS(fStatus, UMDK_FPGALINK_ERR, cleanup, "readMemDirect()");
	retVal = UMDK_SUCCESS;
cleanup:
	return retVal;
}

// Actually do the write:
static void doWrite(struct I68K *self, uint32 address, uint32 length, const uint8 *buf) {
	if ( length ) {
		struct UMDK *umdk = (struct UMDK *)self;
		const char *error;
		UMDKStatus uStatus = writeMemDirect(umdk->fpga, address, length, buf, &error);
		if ( uStatus ) {
			fprintf(stderr, "doWrite() failed: %s\n", error);
			flFreeError(error);
		}
	}
}

// Write some data to memory (replacing monitor vectors if necessary):
static void memWrite(struct I68K *self, uint32 address, uint32 length, const uint8 *buf) {
	const char *error;
	address &= 0xFFFFFF;
	if ( address < 0x800000 ) {
		// This memory is directly accessible
		bool illegalVectorOverlap = isOverlapping(address, length, ILLEGAL_VECTOR, 4);
		bool traceVectorOverlap = isOverlapping(address, length, TRACE_VECTOR, 4);
		if ( illegalVectorOverlap || traceVectorOverlap ) {
			// Overwrite the trace and illegal instruction vectors:
			uint8 *paddedBuf = malloc(length + 6);
			uint8 *actualBuf = paddedBuf + 3;
			memcpy(actualBuf, buf, length);
			if ( illegalVectorOverlap ) {
				memcpy(actualBuf + ILLEGAL_VECTOR - address, monitorAddress, 4);
			}
			if ( traceVectorOverlap ) {
				memcpy(actualBuf + TRACE_VECTOR - address, monitorAddress, 4);
			}
			doWrite(self, address, length, actualBuf);
			free(paddedBuf);
		} else {
			doWrite(self, address, length, buf);
		}
	} else {
		// This memory needs to be copied by the MD from an accessible area
		struct UMDK *umdk = (struct UMDK *)self;
		uint8 cmdBlock[CMDBLK_SIZE + length];
		prepareCommand(cmdBlock, CMD_WRITE, address, length);
		prepareRegisters(cmdBlock, &umdk->machineState);
		memcpy(cmdBlock + CMDBLK_SIZE, buf, length);
		if ( writeMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE + length, cmdBlock, &error) ) {
			goto cleanup;
		}

		// Let MD run...
		umdk->flagByte = (uint8)(umdk->flagByte & ~FLAG_HREQ);
		if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
			goto cleanup;
		}
		
		// MD will execute the write command then wait for further instructions
		if ( remoteAcquire(umdk, false, &error) ) {
			goto cleanup;
		}
	}
	return;
cleanup:
	fprintf(stderr, "memRead() failed: %s\n", error);
	flFreeError(error);
}

// Read some data from memory:
static void memRead(struct I68K *self, uint32 address, uint32 length, uint8 *buf) {
	const char *error;
	if ( length ) {
		struct UMDK *umdk = (struct UMDK *)self;
		address &= 0xFFFFFF;
		// TODO:
		//   Actually, if the requested block of memory straddles either of the two boundaries
		//   between directly-addressable cartridge memory and indirectly-addressable onboard
		//   address space, it should probably drop to the indirect addressing method.
		if ( address < 0x800000 ) {
			// This memory is directly accessible
			readMemDirect(umdk->fpga, address, length, buf, &error);
		} else {
			// This memory needs to be copied by the MD into an accessible area
			uint8 cmdBlock[CMDBLK_SIZE + length];
			prepareCommand(cmdBlock, CMD_READ, address, length);
			prepareRegisters(cmdBlock, &umdk->machineState);
			if ( writeMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, &error) ) {
				goto cleanup;
			}

			// Let MD run...
			umdk->flagByte = (uint8)(umdk->flagByte & ~FLAG_HREQ);
			if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
				goto cleanup;
			}
			
			// MD will execute the read command then wait for further instructions
			if ( remoteAcquire(umdk, false, &error) ) {
				goto cleanup;
			}
			if ( readMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE + length, cmdBlock, &error) ) {
				goto cleanup;
			}
			memcpy(buf, cmdBlock + CMDBLK_SIZE, length);
		}
	}
	return;
cleanup:
	fprintf(stderr, "memRead() failed: %s\n", error);
	flFreeError(error);
}

// Set a register to a new value:
static void setReg(struct I68K *self, Register reg, uint32 value) {
	struct UMDK *umdk = (struct UMDK *)self;
	umdk->machineState.value[reg] = value;
}

// Get the current value of a register:
static uint32 getReg(struct I68K *self, Register reg) {
	struct UMDK *umdk = (struct UMDK *)self;
	return umdk->machineState.value[reg];
}

// Execute one instruction and return:
static I68KStatus execStep(struct I68K *self) {
	struct UMDK *umdk = (struct UMDK *)self;
	uint8 cmdBlock[CMDBLK_SIZE];
	const char *error;
	if ( umdk->flagByte & FLAG_RUN ) {
		// MD not in RESET - assume it's awaiting commands
		prepareCommand(cmdBlock, CMD_STEP, 0, 0);
		prepareRegisters(cmdBlock, &umdk->machineState);
		if ( writeMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, &error) ) {
			goto cleanup;
		}
		umdk->flagByte = (uint8)(umdk->flagByte & ~FLAG_HREQ);
		if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
			goto cleanup;
		}
	} else {
		// MD in RESET - just set it running
		umdk->flagByte = (uint8)(umdk->flagByte | FLAG_RUN);
		if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
			goto cleanup;
		}
	}

	// MD will now run until it hits a breakpoint at which point we can reacquire control:
	if ( remoteAcquire(umdk, false, &error) ) {
		goto cleanup;
	}
	if ( readMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, &error) ) {
		goto cleanup;
	}
	retrieveRegisters(&umdk->machineState, cmdBlock);
	return I68K_SUCCESS;
cleanup:
	fprintf(stderr, "execCont(): %s\n", error);
	flFreeError(error);
	return I68K_DEVICE_ERR;
}

// Execute until a breakpoint is hit, or some critical error occurs:
static I68KStatus execCont(struct I68K *self) {
	struct UMDK *umdk = (struct UMDK *)self;
	uint8 cmdBlock[CMDBLK_SIZE];
	const char *error;
	if ( umdk->flagByte & FLAG_RUN ) {
		// MD not in RESET - assume it's awaiting commands
		prepareCommand(cmdBlock, CMD_CONT, 0, 0);
		prepareRegisters(cmdBlock, &umdk->machineState);
		if ( writeMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, &error) ) {
			goto cleanup;
		}
		umdk->flagByte = (uint8)(umdk->flagByte & ~FLAG_HREQ);
		if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
			goto cleanup;
		}
	} else {
		// MD in RESET - just set it running
		umdk->flagByte |= FLAG_RUN;
		if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
			goto cleanup;
		}
	}
	
	// MD will now run until it hits a breakpoint at which point we can reacquire control:
	if ( remoteAcquire(umdk, false, &error) ) {
		goto cleanup;
	}
	if ( readMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, &error) ) {
		goto cleanup;
	}
	retrieveRegisters(&umdk->machineState, cmdBlock);
	return I68K_SUCCESS;
cleanup:
	fprintf(stderr, "execCont(): %s\n", error);
	flFreeError(error);
	return I68K_DEVICE_ERR;
}

static void wrapRemoteAcquire(struct I68K *self) {
	struct UMDK *umdk = (struct UMDK *)self;
	uint8 cmdBlock[CMDBLK_SIZE];
	remoteAcquire(umdk, true, NULL);
	readMemDirect(umdk->fpga, CMDBLK_BASE, CMDBLK_SIZE, cmdBlock, NULL);
	retrieveRegisters(&umdk->machineState, cmdBlock);
}

static void destroy(struct I68K *self) {
	struct UMDK *umdk = (struct UMDK *)self;
	flClose(umdk->fpga);
	free(umdk);
}

static const struct I68K_VT vt = {
	memWrite,
	memRead,
	setReg,
	getReg,
	execStep,
	execCont,
	wrapRemoteAcquire,
	destroy
};

// Initialise the UMDK connection
struct I68K *umdkCreate(
	const char *vp, const char *ivp, const char *xsvfFile, const char *rom, bool startRunning,
	uint32 brkAddress, IsIntFunc isInterrupted)
{
	struct UMDK *umdk = NULL;
	struct FLContext *fpga = NULL;
	const char *error;
	uint16 vid, pid;
	bool flag;
	uint8 *romData = NULL;
	uint32 romLength;
	uint32 vblankAddress = 0x000000;
	if ( flOpen(vp, &fpga, NULL) ) {
		int count = 100;
		if ( flLoadStandardFirmware(ivp, vp, &error) ) {
			goto cleanup;
		}
		do {
			flSleep(100);
			if ( flIsDeviceAvailable(vp, &flag, &error) ) {
				goto cleanup;
			}
			count--;
		} while ( !flag && count );
		if ( !flag ) {
			fprintf(stderr, "umdkCreate(): Device did not renumerate properly\n");
			return NULL;
		}
		if ( flOpen(vp, &fpga, &error) ) {
			goto cleanup;
		}
	}
	umdk = (struct UMDK *)calloc(sizeof(struct UMDK), 1);
	umdk->vt = &vt;
	umdk->isInterrupted = isInterrupted;
	umdk->fpga = fpga;
	umdk->flagByte = 0x00;
	if ( xsvfFile ) {
		// Don't care whether the FPGA is running or not, load it anyway:
		if ( flProgram(fpga, xsvfFile, NULL, &error) ) {
			goto cleanup;
		}
	} else {
		if ( flIsFPGARunning(fpga, &flag, &error) ) {
			goto cleanup;
		}
		if ( !flag ) {
			// The FPGA is not running!
			errRender(&error, "umdkCreate(): The FPGA is not running. Try --xsvf=<xsvfFile>");
			goto cleanup;
		}
	}
	if ( flWriteChannel(fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
		goto cleanup;
	}
	if ( writeMemDirect(fpga, MONITOR_BASE, monitorCodeSize, monitorCodeData, &error) ) {
		goto cleanup;
	}
	if ( rom ) {
		romData = flLoadFile(rom, &romLength);
		if ( !romData ) {
			errRender(&error, "umdkCreate(): Unable to open rom \"%s\"", rom);
			goto cleanup;
		}
		writeLong(MONITOR_BASE, romData+0x10);   // illegal instruction vector
		writeLong(MONITOR_BASE, romData+0x24);   // trace vector
		vblankAddress = readLong(romData+0x78);  // vblank vector
		if ( writeMemDirect(fpga, 0x000000, romLength, romData, &error) ) {
			flFreeFile(romData);
			goto cleanup;
		}
		flFreeFile(romData);
		if ( startRunning ) {
			umdk->flagByte |= FLAG_RUN;
			if ( flWriteChannel(umdk->fpga, 100, FLAGS_REG, 1, &umdk->flagByte, &error) ) {
				goto cleanup;
			}
		}
	}
	if ( brkAddress ) {
		umdk->brkAddress = brkAddress;
		if ( setBreakpointAddress(fpga, brkAddress, false, &error) ) {
			goto cleanup;
		}
	}
	if ( vblankAddress ) {
		umdk->brkAddress = vblankAddress;
		if ( setBreakpointAddress(fpga, vblankAddress, false, &error) ) {
			goto cleanup;
		}
	}
	return (struct I68K *)umdk;
cleanup:
	fprintf(stderr, "umdkCreate(): %s\n", error);
	flFreeError(error);
	if ( umdk ) {
		umdk->vt->destroy((struct I68K *)umdk);
	}
	return NULL;
}
