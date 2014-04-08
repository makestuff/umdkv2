// This module has just one entry point:
//   int processMessage(const char *buf, int size, int conn)
//     buf - an incoming GDB remote message.
//     size - the number of bytes in the message.
//     conn - the file handle to write the response to: can be a TCP socket or something else.
//     handle - an FPGALink handle
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <makestuff.h>
#include "remote.h"
#include "mem.h"

// Hex digits used in cmdReadMemory() and checksum():
static const char hexDigits[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

// The register names, used for debugging
static const char *regNames[] = {
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
	"A0", "A1", "A2", "A3", "A4", "A5", "FP", "SP",
	"SR", "PC"
};

// GDB remote protocol standard responses:
#define RESPONSE_OK    "+$OK#9A"
#define RESPONSE_EMPTY "+$#00"
#define RESPONSE_SIG   "+$S05#B8"
#define VL(x) x, (sizeof(x)-1)

// Breakpoint stuff:
#define NUM_BRKPOINTS 8
struct BreakInfo {
	uint32 addr;
	uint16 save;
};
static struct BreakInfo breakpoints[] = {
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000},
	{0x00000000, 0x0000}
};

// Parse a series of somechar-separated hex numbers
// e.g parseList("a9,0a:cafe", NULL, &v1, ',', &v2, ':', &v3, '\0', NULL);
// Remember the NULL at the end!
static int parseList(const char *buf, const char **endPtr, ...) {
	char *end;
	va_list vl;
	uint32 *ptr;
	char sep;
	va_start(vl, endPtr);
	ptr = va_arg(vl, uint32*);
	while ( ptr ) {
		sep = (char)va_arg(vl, int);
		*ptr = strtoul(buf, &end, 16);
		if ( !end || *end != sep ) {
			return -1;
		}
		buf = end + 1;
		ptr = va_arg(vl, uint32*);
	}
	if ( endPtr ) {
		*endPtr = buf;
	}
	return 0;
}

// Calculate the checksum of a #-terminated series of chars. Write the two-digit checksum after it
static void checksum(char *message) {
	uint8 checksum = 0;
	while ( *message != '#' ) {
		checksum = (uint8)(checksum + *message);
		message++;
	}
	message++;
	*message++ = hexDigits[checksum >> 4];
	*message = hexDigits[checksum & 0x0F];
}

// TODO: Fix debug/error handling
static const char *g_error = NULL;
#define CHKERR(s) do { if ( s ) { if ( g_error ) { printf("%s\n", g_error); flFreeError(g_error); g_error = NULL; } else { printf("Error code %d\n", status); } } } while(0)

// Process GDB write-register command
static int cmdWriteRegister(const char *cmd, int conn, struct FLContext *handle) {
	char *end;
	uint32 reg;
	uint32 val;
	int status;
	reg = strtoul(cmd, &end, 16);
	if ( !end || *end != '=' ) {
		return -1;
	}
	if ( reg < 18 ) {
		cmd = end + 1;
		val = strtoul(cmd, NULL, 16);
		printf("cmdWriteRegister(%s, 0x%08X)\n", regNames[reg], val);
		status = umdkSetRegister(handle, reg, val, &g_error);
		CHKERR(status);
	} else {
		printf("cmdWriteRegister(?)\n");
	}
	return write(conn, VL(RESPONSE_OK));
}

// Process GDB read-register command
static int cmdReadRegister(const char *cmd, int conn, struct FLContext *handle) {
	uint32 reg, val;
	char response[13];
	int status;
	reg = strtoul(cmd, NULL, 16);
	if ( reg < 18 ) {
		status = umdkGetRegister(handle, reg, &val, &g_error);
		CHKERR(status);
		printf("cmdReadRegister(%s) = 0x%08X)\n", regNames[reg], val);
		sprintf(response, "+$%08X#", val);
		checksum(response + 2);
		return write(conn, response, 13);
	} else {
		return write(conn, VL(RESPONSE_EMPTY));
	}
}

// Process GDB read-all-registers command
static int cmdReadRegisters(int conn, struct FLContext *handle) {
	char response[2+8*18+3];
	struct Registers regs;
	int status = umdkRemoteAcquire(handle, &regs, &g_error);
	CHKERR(status);
	printf(
		"cmdReadRegisters() = {\n\t0x%08X, 0x%08X, 0x%08X, 0x%08X,  0x%08X, 0x%08X, 0x%08X, 0x%08X,\n\t0x%08X, 0x%08X, 0x%08X, 0x%08X,  0x%08X, 0x%08X, 0x%08X, 0x%08X,\n\t0x%08X, 0x%08X\n}\n",
		regs.d0, regs.d1, regs.d2, regs.d3, regs.d4, regs.d5, regs.d6, regs.d7,
		regs.a0, regs.a1, regs.a2, regs.a3, regs.a4, regs.a5, regs.fp, regs.sp,
		regs.sr, regs.pc
	);
	sprintf(
		response, "+$%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X#",
		regs.d0, regs.d1, regs.d2, regs.d3, regs.d4, regs.d5, regs.d6, regs.d7,
		regs.a0, regs.a1, regs.a2, regs.a3, regs.a4, regs.a5, regs.fp, regs.sp,
		regs.sr, regs.pc
	);
	checksum(response + 2);
	return write(conn, response, 2+8*18+3);
}

// Process GDB write-memory command
static int cmdWriteMemory(const char *cmd, int conn, struct FLContext *handle) {
	uint32 address, length, numBytes;
	uint8 ioBuf[SOCKET_BUFFER_SIZE], *binary = ioBuf, byte;
	int status;
	const char *binPtr;
	if ( parseList(cmd, &binPtr, &address, ',', &length, ':', NULL) ) {
		return -1;
	}
	numBytes = length;
	while ( numBytes-- ) {
		byte = (uint8)*binPtr++;
		if ( byte == '}' ) {
			// An escaped byte follows; unescape it
			byte = (uint8)(*binPtr++ | 0x20);
		}
		*binary++ = byte;
	}
	if ( address & 1 || length & 1 ) {
		printf("Nonaligned write: %du bytes to 0x%08X\n", length, address);
	}
	printf("cmdWriteMemory(0x%08X, 0x%08X)\n", address, length);
	status = umdkWriteBytes(handle, address, length, ioBuf, &g_error);
	CHKERR(status);
	return write(conn, VL(RESPONSE_OK));
}

// Process GDB read-memory command
static int cmdReadMemory(const char *cmd, int conn, struct FLContext *handle) {
	uint32 address, length;
	char msgBuf[SOCKET_BUFFER_SIZE];
	char *textPtr;
	uint8 ioBuf[SOCKET_BUFFER_SIZE];
	const uint8 *binPtr;
	uint8 checksum, byte;
	int status;
	if ( parseList(cmd, NULL, &address, ',', &length, '\0', NULL) ) {
		return -8;
	}
	if ( address & 1 || length & 1 ) {
		printf("Nonaligned read: %du bytes from 0x%08X\n", length, address);
	}
	printf("cmdReadMemory(0x%08X, 0x%08X)\n", address, length);
	status = umdkReadBytes(handle, address, length, ioBuf, &g_error);
	CHKERR(status);
	binPtr = ioBuf;
	textPtr = msgBuf;
	checksum = 0;
	*textPtr++ = '+';
	*textPtr++ = '$';
	while ( length-- ) {
		byte = *binPtr++;
		textPtr[0] = hexDigits[byte >> 4];
		textPtr[1] = hexDigits[byte & 0x0F];
		checksum = (uint8)(checksum + textPtr[0]);
		checksum = (uint8)(checksum + textPtr[1]);
		textPtr += 2;
	}
	*textPtr++ = '#';
	*textPtr++ = hexDigits[checksum >> 4];
	*textPtr++ = hexDigits[checksum & 0x0F];
	return write(conn, msgBuf, (size_t)(textPtr-msgBuf));
}

// Process GDB create-breakpoint command
static int cmdCreateBreakpoint(const char *cmd, int conn, struct FLContext *handle) {
	uint32 type, addr, kind;
	int i, status;
	if ( parseList(cmd, NULL, &type, ',', &addr, ',', &kind, '\0', NULL) ) {
		return -1;
	}
	if ( type != 0 ) {
		return -2;
	}

	// Make sure there isn't already a breakpoint at this address
	for ( i = 0; i < NUM_BRKPOINTS; i++ ) {
		if ( breakpoints[i].addr == addr ) {
			return -3;
		}
	}

	// Find a free slot
	i = 0;
	while ( i < NUM_BRKPOINTS && breakpoints[i].addr ) {
		i++;
	}
	if ( i == NUM_BRKPOINTS ) {
		return -4;
	}

	// Insert the breakpoint into the free slot:
	breakpoints[i].addr = addr;
	status = umdkReadWord(handle, addr, &breakpoints[i].save, &g_error);
	CHKERR(status);
	status = umdkWriteWord(handle, addr, ILLEGAL, &g_error);
	CHKERR(status);
	printf("cmdCreateBreakpoint(0x%08X)\n", addr);
	return write(conn, VL(RESPONSE_OK));
}

// Process GDB delete-breakpoint command
static int cmdDeleteBreakpoint(const char *cmd, int conn, struct FLContext *handle) {
	uint32 type, addr, kind;
	int i, status;
	if ( parseList(cmd, NULL, &type, ',', &addr, ',', &kind, '\0', NULL) ) {
		return -1;
	}
	if ( type != 0 ) {
		return -2;
	}

	// Find the breakpoint
	for ( i = 0; i < NUM_BRKPOINTS; i++ ) {
		if ( breakpoints[i].addr == addr ) {
			status = umdkWriteWord(handle, addr, breakpoints[i].save, &g_error);
			CHKERR(status);
			printf("cmdDeleteBreakpoint(0x%08X) = OK!\n", addr);
			breakpoints[i].addr = 0x00000000;
			return write(conn, VL(RESPONSE_OK));
		}
	}
	printf("cmdDeleteBreakpoint(0x%08X) = FAIL!\n", addr);
	return -3;
}

// Process GDB execute-step command
static int cmdStep(int conn, struct FLContext *handle) {
	struct Registers regs;
	int status = umdkStep(handle, &regs, &g_error);
	CHKERR(status);
	printf("cmdStep(PC=0x%08X)\n", regs.pc);
	return write(conn, VL(RESPONSE_SIG));
}

// Process GDB execute-continue command
static int cmdContinue(int conn, struct FLContext *handle) {
	struct Registers regs;
	int status = umdkCont(handle, &regs, &g_error);
	CHKERR(status);
	printf("cmdContinue(PC=0x%08X)\n", regs.pc);
	return write(conn, VL(RESPONSE_SIG));
}

// External interface: process incoming GDB RSP message
int processMessage(const char *buf, int size, int conn, struct FLContext *handle) {
	int returnCode = 0;
	const char *const end = buf + size;
	char ch = *buf;
	int status;
	struct Registers regs;
	while ( buf < end && (ch == '$' || ch == '+' || ch == 0x03) ) {
		if ( ch == 0x03 ) {
			printf("GDB gave the interrupt signal!\n");
			status = umdkRemoteAcquire(handle, &regs, &g_error);
			CHKERR(status);
			//returnCode = write(conn, VL(RESPONSE_SIG));
		}
		ch = *++buf;
	}
	if ( buf == end ) {
		return -1;
	}
	switch ( *buf++ ) {
	// Memory read/write:
	case 'X':
		returnCode = cmdWriteMemory(buf, conn, handle);
		break;
	case 'm':
		returnCode = cmdReadMemory(buf, conn, handle);
		break;
	// Register read/write:
	case 'g':
		returnCode = cmdReadRegisters(conn, handle);
		break;
	case 'p':
		returnCode = cmdReadRegister(buf, conn, handle);
		break;
	case 'P':
		returnCode = cmdWriteRegister(buf, conn, handle);
		break;

	// Execution:
	case 's':
		returnCode = cmdStep(conn, handle);
		break;
	case 'c':
		returnCode = cmdContinue(conn, handle);
		break;

	// Breakpoints:
	case 'Z':
		returnCode = cmdCreateBreakpoint(buf, conn, handle);
		break;
	case 'z':
		returnCode = cmdDeleteBreakpoint(buf, conn, handle);
		break;

	// Status:
	case '?':
		returnCode = write(conn, VL(RESPONSE_SIG));
		break;
	default:
		// Everything else not supported:
		returnCode = write(conn, VL(RESPONSE_EMPTY));
	}
	if ( returnCode < 0 ) {
		printf("Message did not process correctly!\n");
		return -4;
	}
	return 0;
}
