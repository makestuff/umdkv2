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
#include "../mem.h"

extern struct FLContext *g_handle;

TEST(Range_testDirectWriteValidations) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE};
	int retVal;

	// Write two bytes to bottom of page 0 (should work)
	retVal = umdkDirectWrite(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 0 (should work)
	retVal = umdkDirectWrite(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 0 (should fail)
	retVal = umdkDirectWrite(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectWrite(g_handle, 0x3FFFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to bottom of page 8 (should work)
	retVal = umdkDirectWrite(g_handle, 0x400000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 8 (should work)
	retVal = umdkDirectWrite(g_handle, 0x47FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 8 (should fail)
	retVal = umdkDirectWrite(g_handle, 0x47FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to an odd address (should fail)
	retVal = umdkDirectWrite(g_handle, 0x000001, 2, bytes, NULL);
	CHECK_EQUAL(2, retVal);

	// Write one byte to an even address (should fail)
	retVal = umdkDirectWrite(g_handle, 0x000000, 1, bytes, NULL);
	CHECK_EQUAL(3, retVal);
}
