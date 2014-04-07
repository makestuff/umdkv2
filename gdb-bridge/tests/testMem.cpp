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
#include <cstring>
#include <UnitTest++.h>
#include <libfpgalink.h>
#include "../mem.h"

extern struct FLContext *g_handle;

using namespace std;

static void printRegs(const struct Registers *regs) {
	printf("D: 0        1        2        3         4        5        6        7\n");
	printf(
		"D: %08X %08X %08X %08X  %08X %08X %08X %08X\n",
		regs->d0, regs->d1, regs->d2, regs->d3, regs->d4, regs->d5, regs->d6, regs->d7);
	printf(
		"A: %08X %08X %08X %08X  %08X %08X %08X %08X\n",
		regs->a0, regs->a1, regs->a2, regs->a3, regs->a4, regs->a5, regs->a6, regs->a7);
	printf("SR=%08X  PC=%08X\n", regs->sr, regs->pc);
}

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
}

TEST(Range_testDirectReadNonAligned) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 ex0[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D, 0xCC}; // even addr, even count
	const uint8 ex1[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC, 0xCC}; // even addr, odd count
	const uint8 ex2[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC}; // odd addr,  even count
	const uint8 ex3[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xCC, 0xCC}; // odd addr,  odd count
	uint8 buf[8];
	int retVal;

	// Write eight bytes to page 0 (setup)
	retVal = umdkDirectWriteBytes(g_handle, 0x000100, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read six bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkDirectReadBytes(g_handle, 0x000102, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex0, buf, 8);

	// Read five bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkDirectReadBytes(g_handle, 0x000102, 5, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex1, buf, 8);

	// Read six bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkDirectReadBytes(g_handle, 0x000101, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex2, buf, 8);

	// Read five bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkDirectReadBytes(g_handle, 0x000101, 5, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex3, buf, 8);
}

TEST(Range_testDirectReadbackBytes) {
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

		// Print registers
		printRegs(&regs);

		// Verify we're at the vblank address
		CHECK_EQUAL(regs.pc, vbAddr);

		// Execute remote read of the example ROM, and verify
		retVal = umdkExecuteCommand(g_handle, CMD_READ, 0x000000, exampleLength, NULL, buf, NULL);
		CHECK_EQUAL(0, retVal);
		CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);
		flFreeFile(exampleData);
	}
}

TEST(Range_testIndirectWrite) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 overwrite[] = {0x12, 0x34, 0x56, 0x78};
	const uint8 expected[] = {0xCA, 0xFE, 0x12, 0x34, 0x56, 0x78, 0xF0, 0x0D};
	uint8 buf[8];
	int retVal;

	// Direct-write eight bytes to page 0 (setup)
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFF8, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Overwrite four bytes using indirect write
	retVal = umdkIndirectWriteBytes(g_handle, 0x07FFFA, 4, overwrite, NULL);
	CHECK_EQUAL(0, retVal);

	// Read six bytes from an even address
	retVal = umdkDirectReadBytes(g_handle, 0x07FFF8, 8, buf, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(expected, buf, 8);
}

TEST(Range_testIndirectReadNonAligned) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 ex0[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D, 0xCC}; // even addr, even count
	const uint8 ex1[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC, 0xCC}; // even addr, odd count
	const uint8 ex2[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC}; // odd addr,  even count
	const uint8 ex3[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xCC, 0xCC}; // odd addr,  odd count
	uint8 buf[8];
	int retVal;

	// Write eight bytes to page 0 (setup)
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFF8, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read six bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkIndirectReadBytes(g_handle, 0x07FFFA, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex0, buf, 8);

	// Read five bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkIndirectReadBytes(g_handle, 0x07FFFA, 5, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex1, buf, 8);

	// Read six bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkIndirectReadBytes(g_handle, 0x07FFF9, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex2, buf, 8);

	// Read five bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkIndirectReadBytes(g_handle, 0x07FFF9, 5, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex3, buf, 8);
}

TEST(Range_testStep) {
	int retVal;
	struct Registers regs;
	uint32 oldPC;

	// Load vblank address (should be current PC)
	retVal = umdkDirectReadLong(g_handle, VB_VEC, &oldPC, NULL);
	CHECK_EQUAL(0, retVal);

	// Step one instruction
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK(regs.pc != oldPC);
	
	// Step one instruction
	oldPC = regs.pc;
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK(regs.pc != oldPC);
	
	// Step one instruction
	oldPC = regs.pc;
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK(regs.pc != oldPC);
	
	// Step one instruction
	oldPC = regs.pc;
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK(regs.pc != oldPC);
}

TEST(Range_testCont) {
	int retVal;
	struct Registers regs;
	uint32 oldVB;

	// Load vblank address (should be current PC)
	retVal = umdkDirectReadLong(g_handle, VB_VEC, &oldVB, NULL);
	CHECK_EQUAL(0, retVal);

	// Continue
	retVal = umdkCont(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK(regs.pc == oldVB);
}
