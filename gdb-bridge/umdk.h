#ifndef UMDK_H
#define UMDK_H

#include <makestuff.h>
#include "i68k.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct I68K *umdkCreate(
		const char *vp, const char *ivp, const char *xsvfFile, const char *rom, bool startRunning,
		uint32 brkAddress, IsIntFunc isInterrupted
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif
