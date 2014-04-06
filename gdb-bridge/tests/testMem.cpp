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
#include <cstdio>
#include <UnitTest++.h>
#include <libfpgalink.h>
#include "../mem.h"

extern struct FLContext *g_handle;

using namespace std;

TEST(Range_testDirectWriteBytesValidations) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE};
	int retVal;

	// Write two bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 0 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 0 (should fail)
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectWriteBytes(g_handle, 0x3FFFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to bottom of page 8 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x400000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 8 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x47FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 8 (should fail)
	retVal = umdkDirectWriteBytes(g_handle, 0x47FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to an odd address (should fail)
	retVal = umdkDirectWriteBytes(g_handle, 0x000001, 2, bytes, NULL);
	CHECK_EQUAL(2, retVal);

	// Write one byte to an even address (should fail)
	retVal = umdkDirectWriteBytes(g_handle, 0x000000, 1, bytes, NULL);
	CHECK_EQUAL(3, retVal);
}

TEST(Range_testDirectReadBytesValidations) {
	uint8 bytes[4];
	int retVal;

	// Read two bytes to bottom of page 0 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes to top of page 0 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 0 (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x3FFFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes to bottom of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x400000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes to top of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x47FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 8 (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x47FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes to an odd address (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x000001, 2, bytes, NULL);
	CHECK_EQUAL(2, retVal);

	// Read one byte to an even address (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x000000, 1, bytes, NULL);
	CHECK_EQUAL(3, retVal);
}

TEST(Range_testDirectReadBytesback) {
	const uint8 bytes[] = {0xDE, 0xAD, 0xF0, 0x0D};
	uint8 readback[4];
	int retVal;

	// Write four bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x000000, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectReadBytes(g_handle, 0x000000, 4, readback, NULL);
	CHECK_EQUAL(0, retVal);

	// Compare the response
	CHECK_ARRAY_EQUAL(bytes, readback, 4);

	// Write four bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteBytes(g_handle, 0x400400, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectReadBytes(g_handle, 0x400400, 4, readback, NULL);
	CHECK_EQUAL(0, retVal);
	//printf("Got: 0x%02X%02X%02X%02X\n", readback[0], readback[1], readback[2], readback[3]);

	// Compare the response
	CHECK_ARRAY_EQUAL(bytes, readback, 4);
}

// The monitor object code (assembled from 68000 source)
extern const uint32 monitorCodeSize;
extern const uint8 monitorCodeData[];

TEST(Range_testStartMonitor) {
	int retVal;
	uint8 buf[0x8000];
	uint32 vbAddr;
	uint16 oldOp, cmdFlag;
	uint8 *exampleData;
	size_t exampleLength;

	// Put MD in RESET
	buf[0] = 1;
	retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Load example ROM image
	retVal = umdkDirectWriteFile(g_handle, 0x000000, "../../m68k/example/example.bin", NULL);
	CHECK_EQUAL(0, retVal);

	exampleData = flLoadFile("../../m68k/example/example.bin", &exampleLength);
	CHECK(exampleData);

	if ( exampleData ) {
		// Load the monitor image
		retVal = umdkDirectWriteBytes(g_handle, 0x400000, monitorCodeSize, monitorCodeData, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Clear command area
		retVal = umdkDirectWriteLong(g_handle, 0x400400, 0, NULL); // cmdFlag & cmdIndex
		CHECK_EQUAL(0, retVal);
		retVal = umdkDirectWriteLong(g_handle, 0x400404, 0, NULL); // address
		CHECK_EQUAL(0, retVal);
		retVal = umdkDirectWriteLong(g_handle, 0x400408, 0, NULL); // length
		CHECK_EQUAL(0, retVal);
		
		// Release MD from RESET
		buf[0] = 0;
		retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Read address of VDP vertical interrupt vector
		retVal = umdkDirectReadLong(g_handle, 0x78, &vbAddr, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Read opcode at vbAddr
		retVal = umdkDirectReadWord(g_handle, vbAddr, &oldOp, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Replace opcode at vbAddr with illegal instruction, causing MD to enter monitor
		retVal = umdkDirectWriteWord(g_handle, vbAddr, 0x4AFC, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Wait for monitor to start
		do {
			retVal = umdkDirectReadWord(g_handle, 0x400400, &cmdFlag, NULL);
			CHECK_EQUAL(0, retVal);
		} while ( cmdFlag == 0x0000 );
		
		// Write old opcode back vbAddr
		retVal = umdkDirectWriteWord(g_handle, vbAddr, oldOp, NULL);
		
		// Kick off read command
		retVal = umdkDirectWriteWord(g_handle, 0x400402, 2, NULL); // cmdIndex (2 = READ)
		CHECK_EQUAL(0, retVal);
		retVal = umdkDirectWriteLong(g_handle, 0x400404, 0, NULL); // address
		CHECK_EQUAL(0, retVal);
		retVal = umdkDirectWriteLong(g_handle, 0x400408, exampleLength, NULL); // length
		CHECK_EQUAL(0, retVal);
		retVal = umdkDirectWriteWord(g_handle, 0x400400, 2, NULL); // cmdFlag (2 = EXECUTE)
		CHECK_EQUAL(0, retVal);
		
		// Wait for read execution to complete
		do {
			retVal = umdkDirectReadWord(g_handle, 0x400400, &cmdFlag, NULL);
			CHECK_EQUAL(0, retVal);
		} while ( cmdFlag == 0x0002 );
		
		// Fetch result & verify
		retVal = umdkDirectReadBytes(g_handle, 0x400454, (uint32)exampleLength, buf, NULL);
		CHECK_EQUAL(0, retVal);
		CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);
		flFreeFile(exampleData);
	}
}
