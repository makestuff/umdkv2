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
#ifndef REGISTERS_H
#define REGISTERS_H

#include <types.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define DATA_REG   0x00
	#define ADDR0_REG  0x01
	#define ADDR1_REG  0x02
	#define ADDR2_REG  0x03
	#define FLAGS_REG  0x04
	#define BRK0_REG   0x05
	#define BRK1_REG   0x06
	#define BRK2_REG   0x07

	#define FLAG_RUN   (1<<0)
	#define FLAG_HREQ  (1<<1)
	#define FLAG_HACK  (1<<2)

	#define FLAG_BKEN  (1<<7)

	UMDKStatus umdkWriteRegister(uint8 reg, uint32 count, const uint8 *data);
	UMDKStatus umdkReadRegister(uint8 reg, uint32 count, uint8 *buf);

#ifdef __cplusplus
}
#endif

#endif
