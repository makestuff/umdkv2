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

TEST(Range_testDirectWriteDataValidations) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE};
	int retVal;

	// Write two bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 0 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 0 (should fail)
	retVal = umdkDirectWriteData(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectWriteData(g_handle, 0x3FFFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to bottom of page 8 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x400000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 8 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x47FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 8 (should fail)
	retVal = umdkDirectWriteData(g_handle, 0x47FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to an odd address (should fail)
	retVal = umdkDirectWriteData(g_handle, 0x000001, 2, bytes, NULL);
	CHECK_EQUAL(2, retVal);

	// Write one byte to an even address (should fail)
	retVal = umdkDirectWriteData(g_handle, 0x000000, 1, bytes, NULL);
	CHECK_EQUAL(3, retVal);
}

TEST(Range_testDirectReadValidations) {
	uint8 bytes[4];
	int retVal;

	// Read two bytes to bottom of page 0 (should work)
	retVal = umdkDirectRead(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes to top of page 0 (should work)
	retVal = umdkDirectRead(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 0 (should fail)
	retVal = umdkDirectRead(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectRead(g_handle, 0x3FFFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes to bottom of page 8 (should work)
	retVal = umdkDirectRead(g_handle, 0x400000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes to top of page 8 (should work)
	retVal = umdkDirectRead(g_handle, 0x47FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 8 (should fail)
	retVal = umdkDirectRead(g_handle, 0x47FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes to an odd address (should fail)
	retVal = umdkDirectRead(g_handle, 0x000001, 2, bytes, NULL);
	CHECK_EQUAL(2, retVal);

	// Read one byte to an even address (should fail)
	retVal = umdkDirectRead(g_handle, 0x000000, 1, bytes, NULL);
	CHECK_EQUAL(3, retVal);
}

TEST(Range_testDirectReadback) {
	const uint8 bytes[] = {0xDE, 0xAD, 0xF0, 0x0D};
	uint8 readback[4];
	int retVal;

	// Write four bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x000000, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectRead(g_handle, 0x000000, 4, readback, NULL);
	CHECK_EQUAL(0, retVal);

	// Compare the response
	CHECK_ARRAY_EQUAL(bytes, readback, 4);

	// Write four bytes to bottom of page 0 (should work)
	retVal = umdkDirectWriteData(g_handle, 0x400400, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectRead(g_handle, 0x400400, 4, readback, NULL);
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
	uint16 oldOpcode;
	uint8 *exampleData;
	size_t exampleLength;

	// Put MD in RESET
	buf[0] = 1;
	retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);

	// Load example ROM image
	retVal = umdkDirectWriteFile(g_handle, 0x000000, "../../m68k/example/example.bin", NULL);
	CHECK_EQUAL(0, retVal);

	exampleData = flLoadFile("../../m68k/example/example.bin", &exampleLength);
	CHECK(exampleData);

	// Load the monitor image
	retVal = umdkDirectWriteData(g_handle, 0x400000, monitorCodeSize, monitorCodeData, NULL);
	CHECK_EQUAL(0, retVal);

	// Clear command area
	buf[0] = 0x00; // cmdFlag
	buf[1] = 0x00;
	buf[2] = 0x00; // cmdIndex
	buf[3] = 0x00;
	buf[4] = 0x00; // address = 0
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = 0x00;
	buf[8] = 0x00; // length = 0
	buf[9] = 0x00;
	buf[10] = 0x00;
	buf[11] = 0x00;
	retVal = umdkDirectWriteData(g_handle, 0x400400, 12, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Release MD from RESET
	buf[0] = 0;
	retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);

	// Read address of VDP vertical interrupt vector
	retVal = umdkDirectRead(g_handle, 0x78, 4, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Get vertical blanking interrupt address
	vbAddr = buf[0];
	vbAddr <<= 8;
	vbAddr |= buf[1];
	vbAddr <<= 8;
	vbAddr |= buf[2];
	vbAddr <<= 8;
	vbAddr |= buf[3];

	printf("vbAddr = 0x%08X\n", vbAddr);

	// Read opcode at vbAddr
	retVal = umdkDirectRead(g_handle, vbAddr, 2, buf, NULL);
	oldOpcode = buf[0];
	oldOpcode <<= 8;
	oldOpcode |= buf[1];
	
	// Replace opcode at vbAddr with illegal instruction, causing MD to enter monitor
	buf[0] = 0x4A;
	buf[1] = 0xFC;
	retVal = umdkDirectWriteData(g_handle, vbAddr, 2, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Wait for monitor to start
	do {
		retVal = umdkDirectRead(g_handle, 0x400400, 2, buf, NULL);
		CHECK_EQUAL(0, retVal);
	} while ( buf[0] == 0x00 && buf[1] == 0x00 );

	// Setup read command arguments
	buf[0] = 0x00; // cmdIndex
	buf[1] = 0x02;
	buf[2] = 0x00; // address = 0x000000
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00; // length = 0x008000
	buf[7] = 0x00;
	buf[8] = 0x80;
	buf[9] = 0x00;
	retVal = umdkDirectWriteData(g_handle, 0x400402, 10, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Trigger read
	buf[0] = 0x00;
	buf[1] = 0x02;
	retVal = umdkDirectWriteData(g_handle, 0x400400, 2, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Wait for execution to complete
	do {
		retVal = umdkDirectRead(g_handle, 0x400400, 2, buf, NULL);
		CHECK_EQUAL(0, retVal);
		//printf("Got: 0x%02X%02X\n", buf[0], buf[1]);
	} while ( buf[0] == 0x00 && buf[1] == 0x02 );

	// Read result
	retVal = umdkDirectRead(g_handle, 0x400454, (uint32)exampleLength, buf, NULL);
	CHECK_EQUAL(0, retVal);

	buf[vbAddr+1] = (uint8)oldOpcode;
	oldOpcode >>= 8;
	buf[vbAddr] = (uint8)oldOpcode;
	CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);

	// Write old opcode back vbAddr
	retVal = umdkDirectWriteData(g_handle, vbAddr, 2, buf+vbAddr, NULL);
}
