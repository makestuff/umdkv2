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
#ifndef MEMORY_H
#define MEMORY_H

#include <types.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

	UMDKStatus umdkSetAddress(uint32 address);
	UMDKStatus umdkWriteMemory(uint32 address, uint32 count, const uint8 *data);
	UMDKStatus umdkReadMemory(uint32 address, uint32 count, uint8 *buf);

#ifdef __cplusplus
}
#endif

#endif
