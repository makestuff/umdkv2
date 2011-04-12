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
#include <types.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "hal.h"
#include "bus.h"
#include "memory.h"
#include "breakpoint.h"
#include "monitor.h"
#include "bigendian.h"
#include "error.h"
#include "status.h"

#define CMD_NOP  0
#define CMD_STEP 1
#define CMD_CONT 2
#define CMD_SAVE 3
#define CMD_LOAD 4

#define CONTEXT_SIZE (17*4+2)

#define COMMAND_BASE 0x400400
#define SAVE_BASE (COMMAND_BASE+2)

static uint8 getIntMask(uint16 sr) {
	sr >>= 8;
	sr &= 7;
	sr += '0';
	return (uint8)sr;
}

UMDKStatus umdkMonitorGetMachineState(MachineState *machineState) {
	uint8 command[2];
	uint8 buf[CONTEXT_SIZE];
	uint16 sr;
	UMDKStatus status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkReadMemory(SAVE_BASE, CONTEXT_SIZE, buf);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	machineState->d0 = umdkReadLong(buf+4*0);
	machineState->d1 = umdkReadLong(buf+4*1);
	machineState->d2 = umdkReadLong(buf+4*2);
	machineState->d3 = umdkReadLong(buf+4*3);
	machineState->d4 = umdkReadLong(buf+4*4);
	machineState->d5 = umdkReadLong(buf+4*5);
	machineState->d6 = umdkReadLong(buf+4*6);
	machineState->d7 = umdkReadLong(buf+4*7);

	machineState->a0 = umdkReadLong(buf+4*8);
	machineState->a1 = umdkReadLong(buf+4*9);
	machineState->a2 = umdkReadLong(buf+4*10);
	machineState->a3 = umdkReadLong(buf+4*11);
	machineState->a4 = umdkReadLong(buf+4*12);
	machineState->a5 = umdkReadLong(buf+4*13);
	machineState->a6 = umdkReadLong(buf+4*14);
	machineState->a7 = umdkReadLong(buf+4*15);

	machineState->pc = umdkReadLong(buf+4*16);
	sr = umdkReadWord(buf+4*17);
	machineState->sr.supervisor = (sr & 0x2000);
	machineState->sr.intMask = getIntMask(sr);
	machineState->sr.extend = (sr & 0x0010);
	machineState->sr.negative = (sr & 0x0008);
	machineState->sr.zero = (sr & 0x0004);
	machineState->sr.overflow = (sr & 0x0003);
	machineState->sr.carry = (sr & 0x0001);

#ifdef DEBUG
	printf("PC=%08lX\n", machineState->pc);
	printf("  D0=%08lX; D1=%08lX; D2=%08lX; D3=%08lX; D4=%08lX; D5=%08lX; D6=%08lX; D7=%08lX\n",
		   machineState->d0, machineState->d1, machineState->d2, machineState->d3,
		   machineState->d4, machineState->d5, machineState->d6, machineState->d7);
	printf("  A0=%08lX; A1=%08lX; A2=%08lX; A3=%08lX; A4=%08lX; A5=%08lX; A6=%08lX; A7=%08lX\n",
		   machineState->a0, machineState->a1, machineState->a2, machineState->a3,
		   machineState->a4, machineState->a5, machineState->a6, machineState->a7);
#endif

	umdkWriteWord(CMD_NOP, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkMonitorSingleStep(void) {
	uint8 command[2];
	uint8 buf[CONTEXT_SIZE];
	uint32 pcAddr, brkAddr;
	bool brkEnabled;
	UMDKStatus status;
	status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkGetBreakpointEnabled(&brkEnabled);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( brkEnabled ) {
		status = umdkReadMemory(SAVE_BASE, CONTEXT_SIZE, buf);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		pcAddr = umdkReadLong(buf+4*16);
		status = umdkGetBreakpointAddress(&brkAddr);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		if ( pcAddr == brkAddr ) {
			status = umdkSetBreakpointEnabled(false);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			umdkWriteWord(CMD_STEP, command);
			status = umdkWriteMemory(COMMAND_BASE, 2, command);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkRelinquishBus();
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkRequestBus(50, 0UL);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkSetBreakpointEnabled(true);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkRelinquishBus();
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			return UMDK_SUCCESS;
		}
	}
	// Here if brk disabled or if not at brk addr
	umdkWriteWord(CMD_STEP, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkMonitorContinue(void) {
	uint8 command[2];
	uint8 buf[CONTEXT_SIZE];
	uint32 pcAddr, brkAddr;
	bool brkEnabled;
	UMDKStatus status;
	status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkGetBreakpointEnabled(&brkEnabled);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( brkEnabled ) {
		status = umdkReadMemory(SAVE_BASE, CONTEXT_SIZE, buf);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		pcAddr = umdkReadLong(buf+4*16);
		status = umdkGetBreakpointAddress(&brkAddr);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		if ( pcAddr == brkAddr ) {
			status = umdkSetBreakpointEnabled(false);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			umdkWriteWord(CMD_STEP, command);
			status = umdkWriteMemory(COMMAND_BASE, 2, command);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkRelinquishBus();
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkRequestBus(50, 0UL);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			status = umdkSetBreakpointEnabled(true);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
		}
	}
	// Here if brk disabled or if not at brk addr
	umdkWriteWord(CMD_CONT, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkMonitorGetRam(uint8 *buf) {
	uint8 command[2];
	UMDKStatus status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	umdkWriteWord(CMD_SAVE, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	umdkWriteWord(CMD_NOP, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkReadMemory(SAVE_BASE+CONTEXT_SIZE, 0x10000, buf);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkMonitorPutRam(const uint8 *buf) {
	uint8 command[2];
	UMDKStatus status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	umdkWriteWord(CMD_NOP, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRequestBus(50, 0UL);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	umdkWriteWord(CMD_LOAD, command);
	status = umdkWriteMemory(COMMAND_BASE, 2, command);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkWriteMemory(SAVE_BASE+CONTEXT_SIZE, 0x10000, buf);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRelinquishBus();
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}
