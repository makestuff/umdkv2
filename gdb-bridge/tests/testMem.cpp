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
		regs->a0, regs->a1, regs->a2, regs->a3, regs->a4, regs->a5, regs->fp, regs->sp);
	printf("SR=%08X  PC=%08X\n", regs->sr, regs->pc);
}

TEST(foo) {
	struct Registers regs;
	int retVal;
	retVal = umdkDirectWriteFile(g_handle, 0x000000, "../monitor/boot.bin", NULL);
	CHECK_EQUAL(0, retVal);
	retVal = umdkRemoteAcquire(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
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

	// Write two bytes to bottom of page 8 (should work) [comment out to avoid corrupting monitor]
	//retVal = umdkDirectWriteBytes(g_handle, MONITOR, 2, bytes, NULL);
	//CHECK_EQUAL(0, retVal);

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

	// Read two bytes from bottom of page 0 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x000000, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes from top of page 0 (should work)
	retVal = umdkDirectReadBytes(g_handle, 0x07FFFE, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 0 (should fail)
	retVal = umdkDirectReadBytes(g_handle, 0x07FFFE, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read four bytes that span bottom of page 8 (should fail)
	retVal = umdkDirectReadBytes(g_handle, MONITOR-2, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);

	// Read two bytes from bottom of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, MONITOR, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read two bytes from top of page 8 (should work)
	retVal = umdkDirectReadBytes(g_handle, MONITOR+512*1024-2, 2, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read four bytes that span top of page 8 (should fail)
	retVal = umdkDirectReadBytes(g_handle, MONITOR+512*1024-2, 4, bytes, NULL);
	CHECK_EQUAL(1, retVal);
}

/*TEST(Range_testStartMonitor) {
	int retVal;
	uint8 buf[0x8000];
	uint8 *exampleData;
	size_t exampleLength;
	struct Registers regs;

	// Put MD in RESET
	buf[0] = 5;
	retVal = flWriteChannel(g_handle, 1, 1, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Load boot image
	retVal = umdkDirectWriteFile(g_handle, 0x000000, "../monitor/boot.bin", NULL);
	CHECK_EQUAL(0, retVal);

	// Load separately for comparison
	exampleData = flLoadFile("../monitor/boot.bin", &exampleLength);
	CHECK(exampleData);
	if ( exampleData ) {
		// Load the monitor image
		retVal = umdkDirectWriteFile(g_handle, MONITOR, "../monitor/monitor.bin", NULL);
		CHECK_EQUAL(0, retVal);
		
		// It would be good to have a test that compares a trace log for a known command (e.g
		// indirect write of 0xCAFEBABEDEADF00D to 0x470000). Need to comment out the "move.w #1,
		// cmdFlag" line in the monitor code so the monitor actually executes on entry. Then
		// enable tracing below and store trace log. It would have to sanitise the trace FIFO too.
		//
		//retVal = umdkDirectWriteWord(g_handle, CB_INDEX, CMD_WRITE, NULL);
		//CHECK_EQUAL(0, retVal);
		//retVal = umdkDirectWriteLong(g_handle, CB_ADDR, 0x470000, NULL);
		//CHECK_EQUAL(0, retVal);
		//retVal = umdkDirectWriteLong(g_handle, CB_LEN, 8, NULL);
		//CHECK_EQUAL(0, retVal);
		//retVal = umdkDirectWriteLong(g_handle, CB_MEM, 0xCAFEBABE, NULL);
		//CHECK_EQUAL(0, retVal);
		//retVal = umdkDirectWriteLong(g_handle, CB_MEM+4, 0xDEADF00D, NULL);
		//CHECK_EQUAL(0, retVal);
		//retVal = umdkDirectWriteWord(g_handle, CB_FLAG, 2, NULL);
		//CHECK_EQUAL(0, retVal);

		// Clear cmdFlag to ensure remote acquire below doesn't spuriously succeed early
		retVal = umdkDirectWriteWord(g_handle, CB_FLAG, 0, NULL);
		CHECK_EQUAL(0, retVal);

		// Execute readback of the boot ROM, and verify. This is actually necessary to ensure all
		// the previous direct-writes have completed, before releasing the MD from reset. The MD
		// actually takes a long time to come out of reset, so it'll probably work without, but
		// it's safer with it in.
		retVal = umdkDirectReadBytes(g_handle, 0x000000, exampleLength, buf, NULL);
		CHECK_EQUAL(0, retVal);
		CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);
		
		// Release MD from RESET - MD will execute the boot image (incl TMSS) & enter monitor.
		buf[0] = 0; // run, no tracing
		//buf[0] = 2; // run, with tracing
		retVal = flWriteChannelAsync(g_handle, 1, 1, buf, NULL);
		CHECK_EQUAL(0, retVal);
		
		// Read some stuff from the trace-FIFO
		//retVal = flReadChannel(g_handle, 2, 3072, buf, NULL);
		//CHECK_EQUAL(0, retVal);
		//FILE *outFile = fopen("trace.bin", "wb");
		//fwrite(buf, 3072, 1, outFile);
		//fclose(outFile);

		// Acquire monitor
		retVal = umdkRemoteAcquire(g_handle, &regs, NULL);
		CHECK_EQUAL(0, retVal);

		// Print registers
		printRegs(&regs);

		// Execute remote read of the boot ROM, and verify
		memset(buf, 0xCC, 0x8000);
		retVal = umdkExecuteCommand(g_handle, CMD_READ, 0x000000, exampleLength, NULL, buf, NULL, NULL);
		CHECK_EQUAL(0, retVal);
		CHECK_ARRAY_EQUAL(exampleData, buf, exampleLength);
		flFreeFile(exampleData);
	}
}*/

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

TEST(Range_testIndirectReadNonAligned) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 ex0[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D, 0xCC}; // even addr, even count
	const uint8 ex1[]   = {0xCC, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC, 0xCC}; // even addr, odd count
	const uint8 ex2[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0xCC}; // odd addr,  even count
	const uint8 ex3[]   = {0xCC, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xCC, 0xCC}; // odd addr,  odd count
	uint8 buf[8];
	int retVal;
	
	// Do indirect write
	retVal = umdkWriteBytes(g_handle, 0xFF0000, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Blat CB_MEM area to be sure there's no confusion from leftover data
	memset(buf, 0xDD, 8);
	retVal = umdkDirectWriteBytes(g_handle, CB_MEM, 8, buf, NULL);
	CHECK_EQUAL(0, retVal);

	// Verify all eight
	memset(buf, 0xCC, 8);
	retVal = umdkReadBytes(g_handle, 0xFF0000, 8, buf, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(bytes, buf, 8);

	// Read six bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkReadBytes(g_handle, 0xFF0002, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex0, buf, 8);

	// Read five bytes from an even address
	memset(buf, 0xCC, 8);
	retVal = umdkReadBytes(g_handle, 0xFF0002, 5, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex1, buf, 8);

	// Read six bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkReadBytes(g_handle, 0xFF0001, 6, buf+1, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(ex2, buf, 8);

	// Read five bytes from an odd address
	memset(buf, 0xCC, 8);
	retVal = umdkReadBytes(g_handle, 0xFF0001, 5, buf+1, NULL);
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
	retVal = umdkDirectWriteBytes(g_handle, CB_FLAG, 4, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Read them back
	retVal = umdkDirectReadBytes(g_handle, CB_FLAG, 4, readback, NULL);
	CHECK_EQUAL(0, retVal);

	// Compare the response
	CHECK_ARRAY_EQUAL(bytes, readback, 4);
}

TEST(Range_testDirectWriteRead) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 overwrite[] = {0x12, 0x34, 0x56, 0x78};
	const uint8 expected[] = {0xCA, 0xFE, 0x12, 0x34, 0x56, 0x78, 0xF0, 0x0D};
	uint8 buf[8];
	int retVal;

	// Direct-write eight bytes to page 0 (setup)
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFF8, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Overwrite four bytes
	retVal = umdkDirectWriteBytes(g_handle, 0x07FFFA, 4, overwrite, NULL);
	CHECK_EQUAL(0, retVal);

	// Read six bytes from an even address
	retVal = umdkDirectReadBytes(g_handle, 0x07FFF8, 8, buf, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(expected, buf, 8);
}

TEST(Range_testIndirectWriteRead) {
	const uint8 bytes[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xF0, 0x0D};
	const uint8 overwrite[] = {0x12, 0x34, 0x56, 0x78};
	const uint8 expected[] = {0xCA, 0xFE, 0x12, 0x34, 0x56, 0x78, 0xF0, 0x0D};
	uint8 buf[8];
	int retVal;

	// Direct-write eight bytes to page 1 (setup)
	retVal = umdkWriteBytes(g_handle, 0xFF0000, 8, bytes, NULL);
	CHECK_EQUAL(0, retVal);

	// Overwrite four bytes
	retVal = umdkWriteBytes(g_handle, 0xFF0002, 4, overwrite, NULL);
	CHECK_EQUAL(0, retVal);

	// Read six bytes from an even address
	retVal = umdkReadBytes(g_handle, 0xFF0000, 8, buf, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_ARRAY_EQUAL(expected, buf, 8);
}

TEST(Range_testCont) {
	int retVal;
	uint16 oldInsn;
	struct Registers regs;

	// Load test ROM image
	retVal = umdkDirectWriteFile(g_handle, 0x000200, "../monitor/test.bin", NULL);
	CHECK_EQUAL(0, retVal);

	// Set start address
	retVal = umdkSetRegister(g_handle, PC, 0x000200, NULL);
	CHECK_EQUAL(0, retVal);

	// Read word at 0x220 & replace with a breakpoint
	retVal = umdkDirectReadWord(g_handle, 0x220, &oldInsn, NULL);
	CHECK_EQUAL(0, retVal);
	retVal = umdkDirectWriteWord(g_handle, 0x220, ILLEGAL, NULL);
	CHECK_EQUAL(0, retVal);

	// Continue
	retVal = umdkCont(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220, regs.pc);
	CHECK_EQUAL(0, regs.d0);
	CHECK_EQUAL(0, regs.d1);
	CHECK_EQUAL(0, regs.d2);
	CHECK_EQUAL(0, regs.d3);
	CHECK_EQUAL(0, regs.d4);
	CHECK_EQUAL(0, regs.d5);
	CHECK_EQUAL(0, regs.d6);
	CHECK_EQUAL(0, regs.d7);
	CHECK_EQUAL(0, regs.a0);
	CHECK_EQUAL(0, regs.a1);
	CHECK_EQUAL(0, regs.a2);
	CHECK_EQUAL(0, regs.a3);
	CHECK_EQUAL(0, regs.a4);
	CHECK_EQUAL(0, regs.a5);
	CHECK_EQUAL(0, regs.fp);
	CHECK_EQUAL(0, regs.sp);

	retVal = umdkDirectWriteWord(g_handle, 0x220, oldInsn, NULL);
	CHECK_EQUAL(0, retVal);
}

TEST(Range_testStep) {
	int retVal;
	struct Registers regs;

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*1, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*2, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*3, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*4, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*5, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*6, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*7, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*8, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*9, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*10, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*11, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);
	CHECK_EQUAL(0xC2A6A7A4, regs.a2);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*12, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);
	CHECK_EQUAL(0xC2A6A7A4, regs.a2);
	CHECK_EQUAL(0x99DA044E, regs.a3);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*13, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);
	CHECK_EQUAL(0xC2A6A7A4, regs.a2);
	CHECK_EQUAL(0x99DA044E, regs.a3);
	CHECK_EQUAL(0x073763D6, regs.a4);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*14, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);
	CHECK_EQUAL(0xC2A6A7A4, regs.a2);
	CHECK_EQUAL(0x99DA044E, regs.a3);
	CHECK_EQUAL(0x073763D6, regs.a4);
	CHECK_EQUAL(0x32E5A6C1, regs.a5);

	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);
	CHECK_EQUAL(0x220+6*15, regs.pc);
	CHECK_EQUAL(0x410F9343, regs.d0);
	CHECK_EQUAL(0x0E64E2F4, regs.d1);
	CHECK_EQUAL(0x3371A5B6, regs.d2);
	CHECK_EQUAL(0x83B174BA, regs.d3);
	CHECK_EQUAL(0x118C9F22, regs.d4);
	CHECK_EQUAL(0x9E175F2A, regs.d5);
	CHECK_EQUAL(0x804D5E0E, regs.d6);
	CHECK_EQUAL(0x22CA3C6F, regs.d7);
	CHECK_EQUAL(0xAC44C7F0, regs.a0);
	CHECK_EQUAL(0xEFBC0062, regs.a1);
	CHECK_EQUAL(0xC2A6A7A4, regs.a2);
	CHECK_EQUAL(0x99DA044E, regs.a3);
	CHECK_EQUAL(0x073763D6, regs.a4);
	CHECK_EQUAL(0x32E5A6C1, regs.a5);
	CHECK_EQUAL(0x3BCA8003, regs.fp);
}

TEST(Range_testRegGetSet) {
	int retVal;
	struct Registers regs;
	uint32 val;

	// Set D7 to magic value
	retVal = umdkSetRegister(g_handle, D7, 0xCAFEBABE, NULL);
	CHECK_EQUAL(0, retVal);

	// Step one instruction
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);

	// Read D7
	retVal = umdkGetRegister(g_handle, D7, &val, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_EQUAL(0xCAFEBABE, val);

	// Set D7 to magic value
	retVal = umdkSetRegister(g_handle, D7, 0xDEADF00D, NULL);
	CHECK_EQUAL(0, retVal);

	// Step one instruction
	retVal = umdkStep(g_handle, &regs, NULL);
	CHECK_EQUAL(0, retVal);
	printRegs(&regs);

	// Read D7
	retVal = umdkGetRegister(g_handle, D7, &val, NULL);
	CHECK_EQUAL(0, retVal);
	CHECK_EQUAL(0xDEADF00D, val);
}
