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
	retVal = umdkDirectWriteBytes(g_handle, MONITOR-2, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Write two bytes to bottom of page 8 (should work)
	retVal = umdkDirectWriteBytes(g_handle, MONITOR, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write two bytes to top of page 8 (should work)
	retVal = umdkDirectWriteBytes(g_handle, MONITOR+512*1024-2, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Write four bytes that span top of page 8 (should fail)
	retVal = umdkDirectWriteBytes(g_handle, MONITOR+512*1024-2, 4, bytes, NULL);
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
	retVal = umdkDirectReadBytes(g_handle, MONITOR-2, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes to bottom of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, MONITOR, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes to top of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, MONITOR+512*1024-2, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 8 (should fail)
	retVal = umdkDirectReadBytes(g_handle, MONITOR+512*1024-2, 4, bytes, NULL);
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
	retVal = umdkDirectWriteBytes(g_handle, CMD_FLAG, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectReadBytes(g_handle, CMD_FLAG, 4, readback, NULL);
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
	uint8 *exampleData;
	size_t exampleLength;
	struct Registers regs;
	uint32 vbAddr;

	// Put MD in RESET
	buf[0] = 1;
	retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Load example ROM image
	retVal = umdkDirectWriteFile(g_handle, 0x000000, "../../m68k/example/example.bin", NULL);
	CHECK_EQUAL(0, retVal);

	// Load vblank address
	retVal = umdkDirectReadLong(g_handle, VB_VEC, &vbAddr, NULL);
	CHECK_EQUAL(0, retVal);

	// Load separately for comparison
	exampleData = flLoadFile("../../m68k/example/example.bin", &exampleLength);
	CHECK(exampleData);

	if ( exampleData ) {
		// Load the monitor image
		retVal = umdkDirectWriteBytes(g_handle, MONITOR, monitorCodeSize, monitorCodeData, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Clear cmdFlag
		retVal = umdkDirectWriteWord(g_handle, CMD_FLAG, 0, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Release MD from RESET
		buf[0] = 0;
		retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Acquire monitor
		retVal = umdkRemoteAcquire(g_handle, &regs, NULL);
		CHECK_EQUAL(0, retVal);

		printf("D: 0        1        2        3         4        5        6        7\n");
		printf(
			"D: %08X %08X %08X %08X  %08X %08X %08X %08X\n",
			regs.d0, regs.d1, regs.d2, regs.d3, regs.d4, regs.d5, regs.d6, regs.d7);
		printf(
			"A: %08X %08X %08X %08X  %08X %08X %08X %08X\n",
			regs.a0, regs.a1, regs.a2, regs.a3, regs.a4, regs.a5, regs.a6, regs.a7);
		printf("SR=%08X  PC=%08X\n", regs.sr, regs.pc);

		// Verify we're at the vblank address
		CHECK_EQUAL(regs.pc, vbAddr);

		// Execute remote read of the example ROM, and verify
		retVal = umdkExecuteCommand(g_handle, CMD_READ, 0x000000, exampleLength, NULL, buf, NULL);
		CHECK_EQUAL(0, retVal);
		CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);
		flFreeFile(exampleData);
	}
}
