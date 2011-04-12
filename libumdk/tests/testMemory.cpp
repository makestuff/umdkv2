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
#include <UnitTest++.h>
#include <stdio.h>
#include <types.h>
#include "../hal.h"
#include "../reset.h"
#include "../registers.h"
#include "../memory.h"

TEST(Mem_testSetAddress) {
	uint8 byte;
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkSetAddress(0xF00D1E);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkReadRegister(ADDR0_REG, 1, &byte);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0x78, byte);

	status = umdkReadRegister(ADDR1_REG, 1, &byte);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0x06, byte);

	status = umdkReadRegister(ADDR2_REG, 1, &byte);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0x8F, byte);

	umdkClose();
}

TEST(Mem_testOddCount) {
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkSetRunStatus(false);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	
	status = umdkWriteMemory(0x000000, 1, NULL);
	CHECK_EQUAL(UMDK_ALIGNERR, status);

	status = umdkReadMemory(0x000000, 1, NULL);
	CHECK_EQUAL(UMDK_ALIGNERR, status);

	umdkClose();
}

TEST(Mem_testReadBack) {
	const uint8 expected[] = {
		0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87,
		0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};
	uint8 buf[16];
	const uint32 address = 0x3fff00;
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkSetRunStatus(false);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWriteMemory(address, 16, expected);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkReadMemory(address, 16, buf);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(expected, buf, 16);

	umdkClose();
}
