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
#include "registers.h"
#include "status.h"

UMDKStatus umdkSetBreakpointAddress(uint32 address) {
	UMDKStatus status;
	uint8 command[] = {
		BRK0_REG, 0x00, 0x00, 0x00, 0x01, 0x00,
		BRK1_REG, 0x00, 0x00, 0x00, 0x01, 0x00,
		BRK2_REG, 0x00, 0x00, 0x00, 0x01, 0x00,
		BRK0_REG, 0x00, 0x00, 0x00, 0x01, 0x00};
	address >>= 1;
	command[17] = address & 0x000000FF;
	address >>= 8;
	command[11] = address & 0x000000FF;
	address >>= 8;
	command[23] = (address & 0x000000FF) | 0x80;
	status = umdkWrite(command, 6*4, 100);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkGetBreakpointAddress(uint32 *address) {
	UMDKStatus status;
	uint8 byte;
	status = umdkReadRegister(BRK0_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	byte &= ~FLAG_BKEN;
	*address = byte;
	*address <<= 8;
	status = umdkReadRegister(BRK1_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	*address |= byte;
	*address <<= 8;
	status = umdkReadRegister(BRK2_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	*address |= byte;
	*address <<= 1;
	return UMDK_SUCCESS;
}

UMDKStatus umdkSetBreakpointEnabled(bool enabledFlag) {
	UMDKStatus status;
	uint8 byte;
	status = umdkReadRegister(BRK0_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( enabledFlag ) {
		byte |= FLAG_BKEN;
	} else {
		byte &= ~FLAG_BKEN;
	}
	status = umdkWriteRegister(BRK0_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkGetBreakpointEnabled(bool *enabledFlag) {
	UMDKStatus status;
	uint8 byte;
	status = umdkReadRegister(BRK0_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( byte & FLAG_BKEN ) {
		*enabledFlag = true;
	} else {
		*enabledFlag = false;
	}
	return UMDK_SUCCESS;
}
