#ifndef MULTIPLEX_H
#define MULTIPLEX_H

#include "i68k.h"

// Create a multiplexer implementation of the I68K interface, one real, the other simulated. All
// operations done on the multiplexer are separately executed on both, and results compared.
//
DLLEXPORT(struct I68K *) muxCreate(
	struct I68K *emul, struct I68K *umdk
) WARN_UNUSED_RESULT;

#endif
