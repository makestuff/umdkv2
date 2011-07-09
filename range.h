#ifndef RANGE_H
#define RANGE_H

#include <makestuff.h>

#ifdef __cplusplus
extern "C" {
#endif

	bool isOverlapping(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length);
	bool isInside(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length);
	bool isOverlapStart(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length);
	bool isOverlapEnd(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length);
	void patch(uint8 *buf, uint32 targetAddress, uint32 bufSize, const uint8 *patchData, uint32 patchAddress);

#ifdef __cplusplus
}
#endif

#endif
