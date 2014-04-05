#ifndef MEM_H
#define MEM_H

#include <libfpgalink.h>

#ifdef __cplusplus
extern "C" {
#endif

	int umdkDirectWrite(
		struct FLContext *handle, uint32 addr, uint32 len, const uint8 *data, const char **error
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif
