#ifndef MEM_H
#define MEM_H

#include <libfpgalink.h>

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif

#endif
