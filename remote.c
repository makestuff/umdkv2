// This module has just one entry point:
//   int processMessage(const char *buf, int size, int conn)
//     buf - an incoming GDB remote message.
//     size - the number of bytes in the message.
//     conn - the file handle to write the response to: can be a TCP socket or something else.
//     cpu - An instance of the I68K interface: can be an emulator or a real 68000.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <makestuff.h>
#include "i68k.h"
#include "remote.h"

// Hex digits used in cmdReadMemory() and checksum():
static const char hexDigits[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
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
	uint8 save[2];
};
static struct BreakInfo breakpoints[] = {
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}},
	{0x00000000, {0x00, 0x00}}
};
static const uint8 brkOpCode[] = {0x4A, 0xFC};

// Parse a series of somechar-separated hex numbers
// e.g parseList("a9,0a:cafe", NULL, &v1, ',', &v2, ':', &v3, '\0', NULL);
// Remember the NULL at the end!
//
static int parseList(const char *buf, const char **endPtr, ...) {
	char *end;
	va_list vl;
	uint32 *ptr;
	char sep;
	va_start(vl, endPtr);
	ptr = va_arg(vl, uint32*);
	while ( ptr ) {
		sep = va_arg(vl, int);
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
		checksum += *message;
		message++;
	}
	message++;
	*message++ = hexDigits[checksum >> 4];
	*message = hexDigits[checksum & 0x0F];
}

static int cmdWriteRegister(const char *cmd, int conn, struct I68K *cpu) {
	char *end;
	uint32 reg;
	uint32 val;
	reg = strtoul(cmd, &end, 16);
	if ( !end || *end != '=' ) {
		return -1;
	}
	if ( reg < 18 ) {
		cmd = end + 1;
		val = strtoul(cmd, NULL, 16);
		cpu->vt->setReg(cpu, reg, val);
	}
	return write(conn, VL(RESPONSE_OK));
}

static int cmdReadRegister(const char *cmd, int conn, struct I68K *cpu) {
	uint32 reg, val;
	char response[13];
	reg = strtoul(cmd, NULL, 16);
	if ( reg < 18 ) {
		val = cpu->vt->getReg(cpu, reg);
		sprintf(response, "+$%08lX#", val);
		checksum(response + 2);
		return write(conn, response, 13);
	} else {
		return write(conn, VL(RESPONSE_EMPTY));
	}
}

static int cmdReadRegisters(int conn, struct I68K *cpu) {
	char response[2+8*18+3];
	const struct I68K_VT *const vt = cpu->vt;
	sprintf(
		response, "+$%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX%08lX#",
		vt->getReg(cpu, 0), vt->getReg(cpu, 1), vt->getReg(cpu, 2), vt->getReg(cpu, 3),
		vt->getReg(cpu, 4), vt->getReg(cpu, 5), vt->getReg(cpu, 6), vt->getReg(cpu, 7),
		vt->getReg(cpu, 8), vt->getReg(cpu, 9), vt->getReg(cpu, 10), vt->getReg(cpu, 11),
		vt->getReg(cpu, 12), vt->getReg(cpu, 13), vt->getReg(cpu, 14), vt->getReg(cpu, 15),
		vt->getReg(cpu, 16), vt->getReg(cpu, 17)
	);
	checksum(response + 2);
	return write(conn, response, 2+8*18+3);
}

static int cmdWriteMemory(const char *cmd, int conn, struct I68K *cpu) {
	uint32 address, length, numBytes;
	uint8 ioBuf[SOCKET_BUFFER_SIZE], *binary = ioBuf, byte;
	const char *binPtr;
	if ( parseList(cmd, &binPtr, &address, ',', &length, ':', NULL) ) {
		return -1;
	}
	numBytes = length;
	while ( numBytes-- ) {
		byte = *binPtr++;
		if ( byte == '}' ) {
			// An escaped byte follows; unescape it
			byte = *binPtr++ | 0x20;
		}
		*binary++ = byte;
	}
	if ( address & 1 || length & 1 ) {
		printf("Nonaligned write: %lu bytes to 0x%08lX\n", length, address);
	}
	cpu->vt->memWrite(cpu, address, length, ioBuf);
	return write(conn, VL(RESPONSE_OK));
}

static int cmdReadMemory(const char *cmd, int conn, struct I68K *cpu) {
	uint32 address, length;
	char msgBuf[SOCKET_BUFFER_SIZE];
	char *textPtr;
	uint8 ioBuf[SOCKET_BUFFER_SIZE];
	const uint8 *binPtr;
	uint8 checksum, byte;
	if ( parseList(cmd, NULL, &address, ',', &length, '\0', NULL) ) {
		return -8;
	}
	if ( address & 1 || length & 1 ) {
		printf("Nonaligned read: %lu bytes from 0x%08lX\n", length, address);
	}
	cpu->vt->memRead(cpu, address, length, ioBuf);
	binPtr = ioBuf;
	textPtr = msgBuf;
	checksum = 0;
	*textPtr++ = '+';
	*textPtr++ = '$';
	while ( length-- ) {
		byte = *binPtr++;
		textPtr[0] = hexDigits[byte >> 4];
		textPtr[1] = hexDigits[byte & 0x0F];
		checksum += textPtr[0];
		checksum += textPtr[1];
		textPtr += 2;
	}
	*textPtr++ = '#';
	*textPtr++ = hexDigits[checksum >> 4];
	*textPtr++ = hexDigits[checksum & 0x0F];
	return write(conn, msgBuf, textPtr-msgBuf);
}

static int cmdCreateBreakpoint(const char *cmd, int conn, struct I68K *cpu) {
	uint32 type, addr, kind;
	const struct I68K_VT *const vt = cpu->vt;
	int i;
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
	vt->memRead(cpu, addr, 2, breakpoints[i].save);
	vt->memWrite(cpu, addr, 2, brkOpCode);
	return write(conn, VL(RESPONSE_OK));
}

static int cmdDeleteBreakpoint(const char *cmd, int conn, struct I68K *cpu) {
	uint32 type, addr, kind;
	int i;
	if ( parseList(cmd, NULL, &type, ',', &addr, ',', &kind, '\0', NULL) ) {
		return -1;
	}
	if ( type != 0 ) {
		return -2;
	}

	// Find the breakpoint
	for ( i = 0; i < NUM_BRKPOINTS; i++ ) {
		if ( breakpoints[i].addr == addr ) {
			cpu->vt->memWrite(cpu, addr, 2, breakpoints[i].save);
			breakpoints[i].addr = 0x00000000;
			return write(conn, VL(RESPONSE_OK));
		}
	}
	return -3;
}

static int cmdStep(int conn, struct I68K *cpu) {
	if ( cpu->vt->execStep(cpu) ) {
		return -1;
	}
	return write(conn, VL(RESPONSE_SIG));
}

static int cmdContinue(int conn, struct I68K *cpu) {
	if ( cpu->vt->execCont(cpu) ) {
		return -1;
	}
	return write(conn, VL(RESPONSE_SIG));
}

int processMessage(const char *buf, int size, int conn, struct I68K *cpu) {
	int returnCode = 0;
	const char *const end = buf + size;
	char ch = *buf;
	while ( buf < end && (ch == '$' || ch == '+' || ch == 0x03) ) {
		if ( ch == 0x03 ) {
			printf("GDB gave the interrupt signal!\n");
			cpu->vt->remoteAcquire(cpu);
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
		returnCode = cmdWriteMemory(buf, conn, cpu);
		break;
	case 'm':
		returnCode = cmdReadMemory(buf, conn, cpu);
		break;
	// Register read/write:
	case 'g':
		returnCode = cmdReadRegisters(conn, cpu);
		break;
	case 'p':
		returnCode = cmdReadRegister(buf, conn, cpu);
		break;
	case 'P':
		returnCode = cmdWriteRegister(buf, conn, cpu);
		break;

	// Execution:
	case 's':
		returnCode = cmdStep(conn, cpu);
		break;
	case 'c':
		returnCode = cmdContinue(conn, cpu);
		break;

	// Breakpoints:
	case 'Z':
		returnCode = cmdCreateBreakpoint(buf, conn, cpu);
		break;
	case 'z':
		returnCode = cmdDeleteBreakpoint(buf, conn, cpu);
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
