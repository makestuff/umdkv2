/* 
 * Copyright (C) 2009 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <UnitTest++.h>
#include "../range.h"

#define OVERLAPPING(r1start, r1length, r2start, r2length)           \
	CHECK(isOverlapping(r1start, r1length, r2start, r2length)); \
	CHECK(isOverlapping(r2start, r2length, r1start, r1length))

#define NOT_OVERLAPPING(r1start, r1length, r2start, r2length)        \
	CHECK(!isOverlapping(r1start, r1length, r2start, r2length)); \
	CHECK(!isOverlapping(r2start, r2length, r1start, r1length))

TEST(Range_testIsOverlapping) {
	// Verify that two obviously non-overlapping ranges yield false:
	NOT_OVERLAPPING(5,1, 10,1);

	// Verify that two adjacent non-overlapping ranges yield false:
	NOT_OVERLAPPING(5,5, 10,5);

	// Verify that two ranges with a one-unit overlap yield true:
	OVERLAPPING(5,6, 10,5);

	// Verify that two obviously overlapping ranges yield true:
	OVERLAPPING(5,10, 10,10);

	// Verify that two subset ranges yield true:
	OVERLAPPING(5,10, 6,8);

	// Verify that identical ranges yield true:
	CHECK(isOverlapping(5,10, 5,10));
}

TEST(Range_testIsInside) {
	CHECK(!isInside(5,1, 10,1));
	CHECK(!isInside(10,1, 5,1));

	CHECK(!isInside(5,10, 10,10));
	CHECK(!isInside(10,10, 5,10));

	CHECK(isInside(5,10, 6,8));
	CHECK(isInside(5,10, 5,10));

	CHECK(!isInside(5,10, 0,4));
	CHECK(!isInside(5,10, 1,4));
	CHECK(!isInside(5,10, 2,4));
	CHECK(!isInside(5,10, 3,4));
	CHECK(!isInside(5,10, 4,4));
	CHECK(isInside(5,10, 5,4));
	CHECK(isInside(5,10, 6,4));
	CHECK(isInside(5,10, 7,4));
	CHECK(isInside(5,10, 8,4));
	CHECK(isInside(5,10, 9,4));
	CHECK(isInside(5,10, 10,4));
	CHECK(isInside(5,10, 11,4));
	CHECK(!isInside(5,10, 12,4));
	CHECK(!isInside(5,10, 13,4));
	CHECK(!isInside(5,10, 14,4));
	CHECK(!isInside(5,10, 15,4));
	CHECK(!isInside(5,10, 16,4));
	CHECK(!isInside(5,10, 17,4));
}

TEST(Range_testIsOverlapStart) {
	CHECK(!isOverlapStart(5,10, 0,4));
	CHECK(!isOverlapStart(5,10, 1,4));
	CHECK(isOverlapStart(5,10, 2,4));
	CHECK(isOverlapStart(5,10, 3,4));
	CHECK(isOverlapStart(5,10, 4,4));
	CHECK(!isOverlapStart(5,10, 5,4));
	CHECK(!isOverlapStart(5,10, 6,4));
	CHECK(!isOverlapStart(5,10, 7,4));
	CHECK(!isOverlapStart(5,10, 8,4));
	CHECK(!isOverlapStart(5,10, 9,4));
	CHECK(!isOverlapStart(5,10, 10,4));
	CHECK(!isOverlapStart(5,10, 11,4));
	CHECK(!isOverlapStart(5,10, 12,4));
	CHECK(!isOverlapStart(5,10, 13,4));
	CHECK(!isOverlapStart(5,10, 14,4));
	CHECK(!isOverlapStart(5,10, 15,4));
	CHECK(!isOverlapStart(5,10, 16,4));
	CHECK(!isOverlapStart(5,10, 17,4));
}

TEST(Range_testIsOverlapEnd) {
	CHECK(!isOverlapEnd(5,10, 0,4));
	CHECK(!isOverlapEnd(5,10, 1,4));
	CHECK(!isOverlapEnd(5,10, 2,4));
	CHECK(!isOverlapEnd(5,10, 3,4));
	CHECK(!isOverlapEnd(5,10, 4,4));
	CHECK(!isOverlapEnd(5,10, 5,4));
	CHECK(!isOverlapEnd(5,10, 6,4));
	CHECK(!isOverlapEnd(5,10, 7,4));
	CHECK(!isOverlapEnd(5,10, 8,4));
	CHECK(!isOverlapEnd(5,10, 9,4));
	CHECK(!isOverlapEnd(5,10, 10,4));
	CHECK(!isOverlapEnd(5,10, 11,4));
	CHECK(isOverlapEnd(5,10, 12,4));
	CHECK(isOverlapEnd(5,10, 13,4));
	CHECK(isOverlapEnd(5,10, 14,4));
	CHECK(!isOverlapEnd(5,10, 15,4));
	CHECK(!isOverlapEnd(5,10, 16,4));
	CHECK(!isOverlapEnd(5,10, 17,4));
}
