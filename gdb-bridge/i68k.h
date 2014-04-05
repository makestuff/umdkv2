#ifndef I68K_H
#define I68K_H

#include <makestuff.h>

typedef enum {
	D0, D1, D2, D3, D4, D5, D6, D7,
	A0, A1, A2, A3, A4, A5,
	FP, SP, SR, PC
} Register;

// Some of these only make sense for some implementations. A real 68000 cannot communicate a
// double bus error for example.
typedef enum {
	I68K_SUCCESS,
	I68K_ADDRESS_ERR,
	I68K_STACKFRAME_ERR,
	I68K_DOUBLE_ERR,
	I68K_INVALID_INSN_ERR,
	I68K_DEVICE_ERR
} I68KStatus;

// The I68K virtual table; one for each implementation of the I68K interface.
struct I68K;
struct I68K_VT {
	void (*memWrite)(struct I68K *self, uint32 address, uint32 length, const uint8 *buf);
	void (*memRead)(struct I68K *self, uint32 address, uint32 length, uint8 *buf);
	void (*setReg)(struct I68K *self, Register reg, uint32 value);
	uint32 (*getReg)(struct I68K *self, Register reg);
	I68KStatus (*execStep)(struct I68K *self);
	I68KStatus (*execCont)(struct I68K *self);
	void (*remoteAcquire)(struct I68K *self);
	void (*destroy)(struct I68K *self);
};

typedef bool (*IsIntFunc)(const struct I68K *);

// The I68K interface. Implementations may use it directly or may return "derived" structs.
#define I68K_MEMBERS const struct I68K_VT *vt; IsIntFunc isInterrupted; int conn

struct I68K {
	I68K_MEMBERS;
};

#endif
