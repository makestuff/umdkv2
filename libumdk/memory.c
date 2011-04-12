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
#include <stdio.h>
#include <types.h>
#include "hal.h"
#include "registers.h"
#include "error.h"
#include "status.h"

UMDKStatus umdkSetAddress(uint32 address) {
	UMDKStatus status;
	uint8 command[] = {
		ADDR0_REG, 0x00, 0x00, 0x00, 0x01, 0x00,
		ADDR1_REG, 0x00, 0x00, 0x00, 0x01, 0x00,
		ADDR2_REG, 0x00, 0x00, 0x00, 0x01, 0x00
	};
	address >>= 1;  // get word address
	command[17] = address & 0x000000FF;
	address >>= 8;
	command[11] = address & 0x000000FF;
	address >>= 8;
	command[5] = address & 0x000000FF;
	status = umdkWrite(command, 6*3, 100);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkWriteMemory(uint32 address, uint32 count, const uint8 *data) {
	UMDKStatus status;
	uint8 flags;
	if ( count & 1 ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkWriteMemory(): cannot write an odd number of bytes!");
		return UMDK_ALIGNERR;
	}
	status = umdkReadRegister(FLAGS_REG, 1, &flags);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( !(flags & FLAG_RUN) || (flags & FLAG_HACK) ) {
		status = umdkSetAddress(address);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		status = umdkWriteRegister(DATA_REG, count, data);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		return UMDK_SUCCESS;
	} else {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkWriteMemory(): bus not under host control!");
		return UMDK_BUSERR;
	}

}

UMDKStatus umdkReadMemory(uint32 address, uint32 count, uint8 *buf) {
	UMDKStatus status;
	uint8 flags;
	if ( count & 1 ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkReadMemory(): cannot read an odd number of bytes!");
		return UMDK_ALIGNERR;
	}
	status = umdkReadRegister(FLAGS_REG, 1, &flags);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( !(flags & FLAG_RUN) || (flags & FLAG_HACK) ) {
		status = umdkSetAddress(address);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		status = umdkReadRegister(DATA_REG, count, buf);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		return UMDK_SUCCESS;
	} else {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkReadMemory(): bus not under host control!");
		return UMDK_BUSERR;
	}
}
