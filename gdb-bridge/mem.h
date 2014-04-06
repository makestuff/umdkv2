#ifndef MEM_H
#define MEM_H

#include <libfpgalink.h>

#ifdef __cplusplus
extern "C" {
#endif

	int umdkDirectWriteData(
		struct FLContext *handle, uint32 address, uint32 count, const uint8 *data, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectWriteFile(
		struct FLContext *handle, uint32 address, const char *fileName, const char **error
	) WARN_UNUSED_RESULT;

	int umdkDirectRead(
		struct FLContext *handle, uint32 address, uint32 count, uint8 *data,
		const char **error
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif
