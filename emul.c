#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <makestuff.h>
#include "starcpu.h"
#include "emul.h"
#include "i68k.h"
#include "range.h"

// The Starscream memory map
#define MEM_SIZE (24*1024*1024)
static uint8 memory[MEM_SIZE];
static struct STARSCREAM_PROGRAMREGION programfetch[] = {
	{0x000000, MEM_SIZE - 1, (uint32)memory},
	{UINT_MAX, UINT_MAX, 0UL}
};
static struct STARSCREAM_DATAREGION readbyte[] = {
	{0x000000, MEM_SIZE - 1, NULL, memory},
	{UINT_MAX, UINT_MAX, NULL, NULL}
};
static struct STARSCREAM_DATAREGION readword[] = {
	{0x000000, MEM_SIZE-1, NULL, memory},
	{UINT_MAX, UINT_MAX, NULL, NULL}
};
static struct STARSCREAM_DATAREGION writebyte[] = {
	{0x000000, MEM_SIZE-1, NULL, memory},
	{UINT_MAX, UINT_MAX, NULL, NULL}
};
static struct STARSCREAM_DATAREGION writeword[] = {
	{0x000000, MEM_SIZE-1, NULL, memory},
	{UINT_MAX, UINT_MAX, NULL, NULL}
};

// The Starscream execution context
extern struct S68000CONTEXT s68000context;

// Addresses of the 68000 registers in the context (used by getReg() & setReg()):
static unsigned *const regAddresses[] = {
	&s68000context.dreg[0], &s68000context.dreg[1], &s68000context.dreg[2], &s68000context.dreg[3],
	&s68000context.dreg[4], &s68000context.dreg[5], &s68000context.dreg[6], &s68000context.dreg[7],
	&s68000context.areg[0], &s68000context.areg[1], &s68000context.areg[2], &s68000context.areg[3],
	&s68000context.areg[4], &s68000context.areg[5], &s68000context.areg[6], &s68000context.areg[7],
	NULL, &s68000context.pc
};

// Set when a breakpoint is hit
static bool hitBreakpoint;

// Handler function called whenever a breakpoint is hit
static void breakPointHandler(void) {
	hitBreakpoint = true;
	s68000releaseTimeslice();
}

#define MONITOR 0x400000
static const uint8 monitorAddress[] = {
	0x00,
	((MONITOR >> 16) & 0xFF),
	((MONITOR >> 8) & 0xFF),
	(MONITOR & 0xFF)
};
#define ILLEGAL_VECTOR 0x10
#define TRACE_VECTOR   0x24

static void doWrite(struct I68K *self, uint32 address, uint32 length, const uint8 *buf) {
	uint32 byteSwapAddr;
	(void)self;
	while ( length-- ) {
		byteSwapAddr = address ^ 1;
		byteSwapAddr &= 0x00FFFFFF;
		memory[byteSwapAddr] = *buf++;
		address++;
	}
}

// Write some data to memory:
static void memWrite(struct I68K *self, uint32 address, uint32 length, const uint8 *buf) {
	bool illegalVectorOverlap;
	bool traceVectorOverlap;
	(void)self;
	illegalVectorOverlap = isOverlapping(address, length, ILLEGAL_VECTOR, 4);
	traceVectorOverlap = isOverlapping(address, length, TRACE_VECTOR, 4);
	if ( illegalVectorOverlap || traceVectorOverlap ) {
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
}

// Read some data from memory:
static void memRead(struct I68K *self, uint32 address, uint32 length, uint8 *buf) {
	uint32 byteSwapAddr;
	(void)self;
	while ( length-- ) {
		byteSwapAddr = address ^ 1;
		byteSwapAddr &= 0x00FFFFFF;
		*buf++ = memory[byteSwapAddr];
		address++;
	}
}

// Set a register to a new value:
static void setReg(struct I68K *self, Register reg, uint32 value) {
	(void)self;
	if ( reg == 16 ) {
		s68000context.sr = (uint16)value;
	} else {
		*regAddresses[reg] = value;
	}
}

// Get the current value of a register:
static uint32 getReg(struct I68K *self, Register reg) {
	(void)self;
	if ( reg == 16 ) {
		return s68000context.sr;
	} else {
		return *regAddresses[reg];
	}
}

static I68KStatus statCode(uint32 status) {
	if ( status == 0x80000000 ) {
		return I68K_SUCCESS;
	} else if ( status == 0x80000001 ) {
		return I68K_ADDRESS_ERR;
	} else if ( status == 0x80000002 ) {
		return I68K_STACKFRAME_ERR;
	} else if ( status == 0xFFFFFFFF ) {
		return I68K_DOUBLE_ERR;
	} else {
		return I68K_INVALID_INSN_ERR;
	}
}

// Execute one instruction and return:
static I68KStatus execStep(struct I68K *self) {
	(void)self;
	return statCode(s68000exec(1));
}

// Execute until a breakpoint is hit, or some critical error occurs:
static I68KStatus execCont(struct I68K *self) {
	uint32 status;
	(void)self;
	hitBreakpoint = false;
	do {
		status = s68000exec(10000);
	} while ( !hitBreakpoint && status == 0x80000000 );
	return statCode(status);
}

static void destroy(struct I68K *self) {
	(void)self;
}

static const struct I68K_VT vt = {
	memWrite,
	memRead,
	setReg,
	getReg,
	execStep,
	execCont,
	NULL,
	destroy
};

static struct I68K emul;

// Initialise the Starscream 68000 emulator:
struct I68K *emulCreate(void) {
	int i;
	memset(memory, 0, MEM_SIZE);
	s68000init();
	for ( i = 0; i < 8; i++ ) {
		s68000context.dreg[i] = 0x00000000;
		s68000context.areg[i] = 0x00000000;
	}
	s68000context.pc = 0x00000000;
	s68000context.sr = 0x0000;
	
	s68000context.s_fetch = programfetch;
	s68000context.u_fetch = programfetch;

	s68000context.s_readbyte = readbyte;
	s68000context.u_readbyte = readbyte;
	s68000context.s_readword = readword;
	s68000context.u_readword = readword;
	s68000context.s_writebyte = writebyte;
	s68000context.u_writebyte = writebyte;
	s68000context.s_writeword = writeword;
	s68000context.u_writeword = writeword;
	s68000context.illegalhandler = breakPointHandler;
	if ( s68000reset() ) {
		return NULL;
	} else {
		emul.vt = &vt;
		return &emul;
	}
}
