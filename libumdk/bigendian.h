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
#ifndef BIGENDIAN_H
#define BIGENDIAN_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

	uint16 umdkReadWord(const uint8 *p);

	uint32 umdkReadLong(const uint8 *p);

	void umdkWriteWord(uint16 value, uint8 *p);

	void umdkWriteLong(uint32 value, uint8 *p);

#ifdef __cplusplus
}
#endif

#endif
