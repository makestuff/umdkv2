#ifndef MEM_H
#define MEM_H

#include <libfpgalink.h>

#ifdef __cplusplus
extern "C" {
#endif

	// 68000 registers:
	struct Registers {
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
		uint32 fp;
		uint32 sp;
		
		uint32 sr;
		uint32 pc;
	};

	typedef enum {
		D0, D1, D2, D3, D4, D5, D6, D7,
		A0, A1, A2, A3, A4, A5, FP, SP,
		SR, PC
	} Register;

	typedef enum {
		CF_RUNNING,
		CF_READY,
		CF_CMD
	} CmdFlag;

	typedef enum {
		CMD_STEP,
		CMD_CONT,
		CMD_READ,
		CMD_WRITE,
		CMD_RESET
	} Command;

	#define ILLEGAL  0x4AFC
	#define IL_VEC   0x000010
	#define TR_VEC   0x000024
	#define VB_VEC   0x000078
	#define MONITOR  0x400000
	#define CB_FLAG  (MONITOR + 0x400)
	#define CB_INDEX (MONITOR + 0x402)
	#define CB_ADDR  (MONITOR + 0x404)
	#define CB_LEN   (MONITOR + 0x408)
	#define CB_REGS  (MONITOR + 0x40C)
	#define CB_MEM   (MONITOR + 0x454)

	// ---------------------------------------------------------------------------------------------
	// Issuing commands
	//
	int umdkRemoteAcquire(
		struct FLContext *handle, struct Registers *regs, const char **error
	) WARN_UNUSED_RESULT;

	int umdkExecuteCommand(
		struct FLContext *handle, Command command, uint32 address, uint32 length,
		const uint8 *sendData, uint8 *recvData, struct Registers *regs,
		const char **error
	) WARN_UNUSED_RESULT;

	// ---------------------------------------------------------------------------------------------
	// Direct read/write operations
	//
	int umdkDirectWriteFile(
		struct FLContext *handle, uint32 address, const char *fileName, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectWriteBytes(
		struct FLContext *handle, uint32 address, uint32 count, const uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectWriteWord(
		struct FLContext *handle, uint32 address, uint16 value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectWriteLong(
		struct FLContext *handle, uint32 address, uint32 value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadBytes(
		struct FLContext *handle, uint32 address, uint32 count, uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadBytesAsync(
		struct FLContext *handle, uint32 address, uint32 count, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadWord(
		struct FLContext *handle, uint32 address, uint16 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadLong(
		struct FLContext *handle, uint32 address, uint32 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	// ---------------------------------------------------------------------------------------------
	// Generic read/write operations (delegate to direct/indirect as needed)
	//
	int umdkWriteBytes(
		struct FLContext *handle, uint32 address, uint32 count, const uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

	int umdkWriteWord(
		struct FLContext *handle, uint32 address, uint16 value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkWriteLong(
		struct FLContext *handle, uint32 address, uint32 value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkReadBytes(
		struct FLContext *handle, uint32 address, uint32 count, uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

	int umdkReadWord(
		struct FLContext *handle, uint32 address, uint16 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	int umdkReadLong(
		struct FLContext *handle, uint32 address, uint32 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	// ---------------------------------------------------------------------------------------------
	// Control flow operations
	//
	int umdkStep(
		struct FLContext *handle, struct Registers *regs, const char **error
	) WARN_UNUSED_RESULT;

	int umdkCont(
		struct FLContext *handle, struct Registers *regs, const char **error
	) WARN_UNUSED_RESULT;

	// ---------------------------------------------------------------------------------------------
	// Register set/get operations
	//
	int umdkSetRegister(
		struct FLContext *handle, Register reg, uint32 value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkGetRegister(
		struct FLContext *handle, Register reg, uint32 *value, const char **error
	) WARN_UNUSED_RESULT;

	int umdkReset(
		struct FLContext *handle, const char **error
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif
