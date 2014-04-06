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
		uint32 a6;
		uint32 a7;
		
		uint32 sr;
		uint32 pc;
	};

	// ---------------------------------------------------------------------------------------------
	// Direct write operations
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

	// ---------------------------------------------------------------------------------------------
	// Direct read operations
	//
	int umdkDirectReadBytes(
		struct FLContext *handle, uint32 address, uint32 count, uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadWord(
		struct FLContext *handle, uint32 address, uint16 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectReadLong(
		struct FLContext *handle, uint32 address, uint32 *pValue, const char **error
	) WARN_UNUSED_RESULT;

	// ---------------------------------------------------------------------------------------------
	// Issuing commands
	//
	int umdkRemoteAcquire(
		struct FLContext *handle, struct Registers *regs, const char **error
	) WARN_UNUSED_RESULT;

	int umdkExecuteCommand(
		struct FLContext *handle, uint16 command, uint32 address, uint32 length,
		const uint8 *sendData, uint8 *recvData, const char **error
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif
