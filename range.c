#include "range.h"

bool isOverlapping(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length) {
	const uint32 r1end = r1start + r1length;
	const uint32 r2end = r2start + r2length;
	if ( r1end <= r2start || r1start >= r2end ) {
		return false;
	} else {
		return true;
	}
}

bool isInside(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length) {
	const uint32 r1end = r1start + r1length;
	const uint32 r2end = r2start + r2length;
	if ( r1start <= r2start && r1end >= r2end ) {
		return true;
	} else {
		return false;
	}
}

bool isOverlapStart(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length) {
	const uint32 r2end = r2start + r2length;
	(void)r1length;
	if ( r2start < r1start && r2end > r1start ) {
		return true;
	} else {
		return false;
	}
}

bool isOverlapEnd(uint32 r1start, uint32 r1length, uint32 r2start, uint32 r2length) {
	const uint32 r1end = r1start + r1length;
	const uint32 r2end = r2start + r2length;
	if ( r2start < r1end && r2end > r1end ) {
		return true;
	} else {
		return false;
	}
}
