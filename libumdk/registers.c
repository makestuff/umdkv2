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
#include "hal.h"
#include "bigendian.h"
#include "error.h"
#include "status.h"

UMDKStatus umdkWriteRegister(uint8 reg, uint32 count, const uint8 *data) {
	UMDKStatus status;
	uint8 command[] = {reg & 0x7F, 0x00, 0x00, 0x00, 0x00};
	umdkWriteLong(count, command+1);
	status = umdkWrite(command, 5, 100);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkWrite(data, count, 100 + count/4096);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkReadRegister(uint8 reg, uint32 count, uint8 *buf) {
	UMDKStatus status;
	uint8 command[] = {reg | 0x80, 0x00, 0x00, 0x00, 0x00};
	umdkWriteLong(count, command+1);
	status = umdkWrite(command, 5, 100);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	status = umdkRead(buf, count, 100 + count/4096);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}
