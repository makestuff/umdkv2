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
#ifndef HAL_H
#define HAL_H

#include <types.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

	// Open a connection to the UMDKv2 device at the specified VID & PID (usually 0x04B4 & 0x8613)
	//
	UMDKStatus umdkOpen(uint16 vid, uint16 pid);

	// Close the connection to the UMDKv2 device
	//
	void umdkClose(void);

	// Write some raw bytes to the UMDKv2. Sync problems (requiring power-cycle to clear) will
	// arise if these bytes are not valid NeoPurgo READ or WRITE commands:
	//   WRITE (six or more bytes):  [Reg,      N, Data0, Data1, ... DataN]
	//   READ (exactly five bytes):  [Reg|0x80, N]
	//     Reg is the FPGA register number (0-127)
	//     N is a big-endian uint32 representing the number of data bytes to read or write
	//
	// Immediately after sending a read command you MUST call umdkRead() with count=N.
	//
	// The timeout should be sufficiently large to actually transfer the number of bytes requested,
	// or sync problems (requiring power-cycle to clear) will arise. A good rule of thumb is
	// 100 + K/10 where K is the number of kilobytes to transfer.
	//
	UMDKStatus umdkWrite(const uint8 *bytes, uint32 count, uint32 timeout);

	// Read some raw bytes from the UMDKv2. Bytes will only be available if they have been
	// previously requested with a NeoPurgo READ command sent with umdkWrite(). The count value
	// should be the same as the actual number of bytes requested by the umdkWrite() READ command.
	//
	// The timeout should be sufficiently large to actually transfer the number of bytes requested,
	// or sync problems (requiring power-cycle to clear) will arise. A good rule of thumb is
	// 100 + K/10 where K is the number of kilobytes to transfer.
	//
	UMDKStatus umdkRead(uint8 *buffer, uint32 count, uint32 timeout);

#ifdef __cplusplus
}
#endif

#endif
