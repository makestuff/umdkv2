#include <libfpgalink.h>
#include <liberror.h>
#include "range.h"

int umdkDirectWrite(
	struct FLContext *handle, uint32 addr, uint32 len, const uint8 *data, const char **error)
{
	int retVal = 0;
	//FLStatus status;
	//uint8 command[8];
	(void)handle;
	(void)data;

	// First verify the write is in a legal range
	//
	if ( isInside(0x400000, 0x80000, addr, len) ) {
		// Write is to the UMDKv2-reserved 512KiB of address-space at 0x400000. The mapping for this
		// is fixed to the top 512KiB of SDRAM, so we need to transpose the MD virtual address to
		// get the correct SDRAM physical address.
		addr += 0xb80000;
	} else if ( !isInside(0, 0x80000, addr, len) ) {
		// The only other region of the address-space which is directly host-addressable is the
		// bottom 512KiB of address-space, which has a logical-physical mapping that is guaranteed
		// by SEGA to be 1:1 (i.e no transpose necessary). So any attempt to directly access memory
		// anywhere else is an error.
		CHECK_STATUS(
			true, 1, cleanup,
			"umdkDirectWrite(): Illegal direct-write to 0x%06X-0x%06X range!", addr, addr+len-1
		);
	}

	// Next verify that the write is to an even address, and has even length
	//
	CHECK_STATUS(addr&1, 2, cleanup, "umdkDirectWrite(): Address must be even!");
	CHECK_STATUS(len&1, 3, cleanup, "umdkDirectWrite(): Length must be even!");
cleanup:
	return retVal;
}
