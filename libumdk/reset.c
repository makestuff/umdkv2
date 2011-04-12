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
#include "registers.h"
#include "error.h"
#include "status.h"

UMDKStatus umdkSetRunStatus(bool runFlag) {
	UMDKStatus status;
	uint8 byte;
	if ( runFlag ) {
		status = umdkReadRegister(FLAGS_REG, 1, &byte);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		byte |= FLAG_RUN;
	} else {
		byte = 0x00;
	}
	status = umdkWriteRegister(FLAGS_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}
