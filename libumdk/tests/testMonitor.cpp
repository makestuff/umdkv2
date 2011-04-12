/* 
 * Copyright (C) 2011 Chris McClelland
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
#include <stdlib.h>
#include <types.h>
#include "../hal.h"
#include "../reset.h"
#include "../bus.h"
#include "../memory.h"
#include "../breakpoint.h"
#include "../monitor.h"
#include "../readfile.h"
#include "../bigendian.h"

TEST(Monitor_testMonitor) {
	uint8 *rom, *mon;
	uint32 romLength, monLength;
	MachineState machineState;
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	rom = umdkReadFile("../../m68k/testrom/testrom.bin", &romLength);
	CHECK(rom);
	mon = umdkReadFile("../../m68k/monitor/monitor.bin", &monLength);
	CHECK(mon);

	umdkWriteLong(0x400000, rom+0x10); // illegal instruction vector                                                                                                                                     
    umdkWriteLong(0x400000, rom+0x24); // trace vector                         

	status = umdkSetRunStatus(false);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWriteMemory(0x000000, romLength, rom);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWriteMemory(0x400000, monLength, mon);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	free(mon);
	free(rom);

	status = umdkSetRunStatus(true);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkSetBreakpointAddress(BRK_ADDR);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	// Just executed move.l #-1, d0: expecting PC=BRK_ADDR, D0=-1, D2=0, SR.N
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(BRK_ADDR, machineState.pc);
	CHECK_EQUAL(0xFFFFFFFFUL, machineState.d0);
	CHECK_EQUAL(0x00000000UL, machineState.d2);
	CHECK_EQUAL(false, machineState.sr.extend);
	CHECK_EQUAL(true, machineState.sr.negative);
	CHECK_EQUAL(false, machineState.sr.zero);
	CHECK_EQUAL(false, machineState.sr.overflow);
	CHECK_EQUAL(false, machineState.sr.carry);

	// Step one instruction
	status = umdkMonitorSingleStep();
	CHECK_EQUAL(UMDK_SUCCESS, status);

	// Just executed asl.l #1, d0: expecting PC=BRK_ADDR+2, D0=-2, D2=0, SR.XNVC
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(BRK_ADDR+2UL, machineState.pc);
	CHECK_EQUAL(0xFFFFFFFEUL, machineState.d0);
	CHECK_EQUAL(0x00000000UL, machineState.d2);
	CHECK_EQUAL(true, machineState.sr.extend);
	CHECK_EQUAL(true, machineState.sr.negative);
	CHECK_EQUAL(false, machineState.sr.zero);
	CHECK_EQUAL(true, machineState.sr.overflow);
	CHECK_EQUAL(true, machineState.sr.carry);
	
	// Step one instruction
	status = umdkMonitorSingleStep();
	CHECK_EQUAL(UMDK_SUCCESS, status);

	// Just executed move.l #0, d0: expecting PC=BRK_ADDR+4, D0=0, D2=0, SR.XZ
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(BRK_ADDR+4UL, machineState.pc);
	CHECK_EQUAL(0x00000000UL, machineState.d0);
	CHECK_EQUAL(0x00000000UL, machineState.d2);
	CHECK_EQUAL(true, machineState.sr.extend);
	CHECK_EQUAL(false, machineState.sr.negative);
	CHECK_EQUAL(true, machineState.sr.zero);
	CHECK_EQUAL(false, machineState.sr.overflow);
	CHECK_EQUAL(false, machineState.sr.carry);


	// Some more single-steps just for good measure
	status = umdkMonitorSingleStep();
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkMonitorSingleStep();
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkMonitorSingleStep();
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);


	// Continue until breakpoint again
	status = umdkMonitorContinue();
	CHECK_EQUAL(UMDK_SUCCESS, status);

	// We're on the next iteration of the loop: expecting PC=BRK_ADDR, D0=0xFFFF, D2=1
	status = umdkMonitorGetMachineState(&machineState);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(BRK_ADDR, machineState.pc);
	//CHECK_EQUAL(0x0000FFFFUL, machineState.d0);
	CHECK_EQUAL(0x00000001UL, machineState.d2);
	CHECK_EQUAL(false, machineState.sr.extend);
	CHECK_EQUAL(false, machineState.sr.negative);
	CHECK_EQUAL(false, machineState.sr.zero);
	CHECK_EQUAL(false, machineState.sr.overflow);
	CHECK_EQUAL(false, machineState.sr.carry);

	status = umdkSetBreakpointEnabled(false);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	umdkClose();
}
