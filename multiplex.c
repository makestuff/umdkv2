#include <makestuff.h>
#include <stdio.h>
#include <string.h>
#include "i68k.h"

struct Multiplex {
	I68K_MEMBERS;
	struct I68K *emul;
	struct I68K *umdk;
};

// Write some data to memory:
static void memWrite(struct I68K *self, uint32 address, uint32 length, const uint8 *buf) {
	struct Multiplex *mux = (struct Multiplex *)self;
	mux->emul->vt->memWrite(mux->emul, address, length, buf);
	mux->umdk->vt->memWrite(mux->umdk, address, length, buf);
}

static void dumpSimple(const unsigned char *input, unsigned int length) {
	while ( length ) {
		printf(" %02X", *input++);
		--length;
	}
}

// Read some data from memory:
static void memRead(struct I68K *self, uint32 address, uint32 length, uint8 *buf) {
	uint8 tmp[1024];
	struct Multiplex *mux = (struct Multiplex *)self;
	mux->emul->vt->memRead(mux->emul, address, length, buf);
	mux->umdk->vt->memRead(mux->umdk, address, length, tmp);
	if ( memcmp(buf, tmp, length) ) {
		printf("memRead(0x%08X, 0x%08X):\n  emul:", address, length);
		dumpSimple(buf, length);
		printf("\n  umdk:");
		dumpSimple(tmp, length);
		printf("\n");
	}
}

// Set a register to a new value:
static void setReg(struct I68K *self, Register reg, uint32 value) {
	struct Multiplex *mux = (struct Multiplex *)self;
	mux->emul->vt->setReg(mux->emul, reg, value);
	mux->umdk->vt->setReg(mux->umdk, reg, value);
}

// Get the current value of a register:
static uint32 getReg(struct I68K *self, Register reg) {
	struct Multiplex *mux = (struct Multiplex *)self;
	uint32 emulValue, umdkValue;
	emulValue = mux->emul->vt->getReg(mux->emul, reg);
	umdkValue = mux->umdk->vt->getReg(mux->umdk, reg);
	if ( emulValue != umdkValue ) {
		printf("getReg(%d):\n  emul: 0x%08X\n  umdk: 0x%08X\n", reg, emulValue, umdkValue);
	}
	return emulValue;
}

// Execute one instruction and return:
static I68KStatus execStep(struct I68K *self) {
	struct Multiplex *mux = (struct Multiplex *)self;
	I68KStatus emulStatus, umdkStatus;
	emulStatus = mux->emul->vt->execStep(mux->emul);  // TODO: WTF is going on here?
	umdkStatus = mux->umdk->vt->execStep(mux->umdk);
	(void)umdkStatus;
	return emulStatus;
}

// Execute until a breakpoint is hit, or some critical error occurs:
static I68KStatus execCont(struct I68K *self) {
	struct Multiplex *mux = (struct Multiplex *)self;
	I68KStatus emulStatus, umdkStatus;
	emulStatus = mux->emul->vt->execCont(mux->emul);  // TODO: WTF is going on here?
	umdkStatus = mux->umdk->vt->execCont(mux->umdk);
	(void)umdkStatus;
	return emulStatus;
}

static void destroy(struct I68K *self) {
	struct Multiplex *mux = (struct Multiplex *)self;
	mux->emul->vt->destroy(mux->emul);
	mux->umdk->vt->destroy(mux->umdk);
}

static const struct I68K_VT vt = {
	memWrite,
	memRead,
	setReg,
	getReg,
	execStep,
	execCont,
	destroy
};

static struct Multiplex mux;

struct I68K *muxCreate(struct I68K *emul, struct I68K *umdk) {
	mux.emul = emul;
	mux.umdk = umdk;
	mux.vt = &vt;
	return (struct I68K *)&mux;
}
