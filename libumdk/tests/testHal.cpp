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

TEST(USB_testOpenClose) {
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_ALREADYOPEN, status);

	umdkClose();
}

TEST(USB_testWrite) {
	const uint8 writeOne[] = {
		ADDR1_REG, 0x00, 0x00, 0x00, 0x01,
		0xAA};
	const uint8 writeTwo[] = {
		ADDR1_REG, 0x00, 0x00, 0x00, 0x02,
		0xAA, 0xAA};
	const uint8 writeFour[] = {
		ADDR1_REG, 0x00, 0x00, 0x00, 0x04,
		0xAA, 0xAA, 0xAA, 0xAA};
	const uint8 writeEight[] = {
		ADDR1_REG, 0x00, 0x00, 0x00, 0x08,
		0xAA, 0xAA, 0xAA, 0xAA,
		0xAA, 0xAA, 0xAA, 0xAA};
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWrite(writeOne, 5+1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWrite(writeTwo, 5+2, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWrite(writeFour, 5+4, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWrite(writeEight, 5+8, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	umdkClose();
}

TEST(USB_testRead) {
	const uint8 writeOne[] = {
		ADDR1_REG, 0x00, 0x00, 0x00, 0x01,
		0xAA};
	const uint8 readOne[] = {0x82, 0x00, 0x00, 0x00, 0x01};
	const uint8 readTwo[] = {0x82, 0x00, 0x00, 0x00, 0x02};
	const uint8 readFour[] = {0x82, 0x00, 0x00, 0x00, 0x04};
	const uint8 readEight[] = {0x82, 0x00, 0x00, 0x00, 0x08};

	uint8 buf[8];
	uint8 exp[8];
	UMDKStatus status = umdkOpen(0x04B4, 0x8613);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	status = umdkWrite(writeOne, 5+1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = 0xAA;
	exp[1] = exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkWrite(readOne, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(buf, 1, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = 0xAA;
	exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkWrite(readTwo, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(buf, 2, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = exp[2] = exp[3] = 0xAA;
	exp[4] = exp[5] = exp[6] = exp[7] = 0x00;
	status = umdkWrite(readFour, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(buf, 4, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = 0x00;
	exp[0] = exp[1] = exp[2] = exp[3] = exp[4] = exp[5] = exp[6] = exp[7] = 0xAA;
	status = umdkWrite(readEight, 5, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	status = umdkRead(buf, 8, 100);
	CHECK_EQUAL(UMDK_SUCCESS, status);
	CHECK_ARRAY_EQUAL(exp, buf, 8);

	umdkClose();
}
