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
#include "../registers.h"

TEST(Regs_testWriteRegister) {
	const uint8 data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	const uint8 readOne[] = {0x82, 0x00, 0x00, 0x00, 0x01};
	uint8 buf;
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWriteRegister(ADDR1_REG, 1, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xAA, buf);

	status = umdkWriteRegister(ADDR1_REG, 2, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xBB, buf);

	status = umdkWriteRegister(ADDR1_REG, 3, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xCC, buf);

	status = umdkWriteRegister(ADDR1_REG, 4, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xDD, buf);

	status = umdkWriteRegister(ADDR1_REG, 5, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xEE, buf);

	status = umdkWriteRegister(ADDR1_REG, 6, data);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(&buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_EQUAL(0xFF, buf);

	umdkClose();
}

TEST(Regs_testReadRegister) {
	const uint8 data = 0xAA;
	uint8 buf[8], exp[8];
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWriteRegister(ADDR1_REG, 1, &data);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = 0xAA;
	exp[1] = exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkReadRegister(ADDR1_REG, 1, buf);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = 0xAA;
	exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkReadRegister(ADDR1_REG, 2, buf);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = exp[2] = exp[3] = 0xAA;
	exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkReadRegister(ADDR1_REG, 4, buf);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0xAA;
	status = umdkReadRegister(ADDR1_REG, 8, buf);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	umdkClose();
}
