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
#ifndef MONITOR_H
#define MONITOR_H

#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		bool supervisor;
		uint8 intMask;
		bool extend;
		bool negative;
		bool zero;
		bool overflow;
		bool carry;
	} StatusRegister;

	typedef struct {
		uint32 d0;
		uint32 d1;
		uint32 d2;
		uint32 d3;
		uint32 d4;
		uint32 d5;
		uint32 d6;
		uint32 d7;

		uint32 a0;
		uint32 a1;
		uint32 a2;
		uint32 a3;
		uint32 a4;
		uint32 a5;
		uint32 a6;
		uint32 a7;

		uint32 pc;
		StatusRegister sr;
	} MachineState;

	UMDKStatus umdkMonitorSingleStep(void);
	UMDKStatus umdkMonitorContinue(void);
	UMDKStatus umdkMonitorGetMachineState(MachineState *machineState);
	UMDKStatus umdkMonitorGetRam(uint8 *buf);
	UMDKStatus umdkMonitorPutRam(const uint8 *buf);

#ifdef __cplusplus
}
#endif

#endif
