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
#define _BSD_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <types.h>
#include "registers.h"
#include "error.h"
#include "status.h"

UMDKStatus umdkRequestBus(uint32 maxTries, uint32 *numTries) {
	UMDKStatus status;
	uint8 byte;
	uint32 tries = 0;
	status = umdkReadRegister(FLAGS_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( !(byte & FLAG_RUN) ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkRequestBus(): MegaDrive not running!");
		return UMDK_BUSERR;
	}

	if ( !(byte & FLAG_HACK) ) {
		byte |= FLAG_HREQ;
		status = umdkWriteRegister(FLAGS_REG, 1, &byte);
		if ( status != UMDK_SUCCESS ) {
			return status;
		}
		do {
			status = umdkReadRegister(FLAGS_REG, 1, &byte);
			if ( status != UMDK_SUCCESS ) {
				return status;
			}
			tries++;
			usleep(100000);
		} while ( tries < maxTries && !(byte & FLAG_HACK) );
		
		if ( numTries ) {
			*numTries = tries;
		}
		
		if ( byte & FLAG_HACK ) {
			return UMDK_SUCCESS;
		} else {
			snprintf(
				m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
				"umdkRequestBus(): MegaDrive not responding!");
			return UMDK_TIMEOUT;
		}
	} else {
		if ( numTries ) {
			*numTries = 0;
		}
		return UMDK_SUCCESS;
	}
}

UMDKStatus umdkRelinquishBus(void) {
	UMDKStatus status;
	uint8 byte;
	status = umdkReadRegister(FLAGS_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	if ( !(byte & FLAG_RUN) ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkRequestBus(): MegaDrive not running!");
		return UMDK_BUSERR;
	}
	if ( !(byte & FLAG_HREQ) ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkRequestBus(): there is no bus request active!");
		return UMDK_BUSERR;
	}
	byte &= ~FLAG_HREQ;
	status = umdkWriteRegister(FLAGS_REG, 1, &byte);
	if ( status != UMDK_SUCCESS ) {
		return status;
	}
	return UMDK_SUCCESS;
}
