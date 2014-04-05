/* 
 * Copyright (C) 2009 Chris McClelland
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
#include <iostream>
#include <UnitTest++.h>
#include <libfpgalink.h>

using namespace std;

struct FLContext *g_handle = NULL;

int main() {
	int retVal = 0;
	int testResult;
	FLStatus status;
	status = flInitialise(0, NULL);
	CHECK_STATUS(status, -1, cleanup);
	status = flOpen("1d50:602b", &g_handle, NULL);
	CHECK_STATUS(status, -2, cleanup);
	status = flSelectConduit(g_handle, 0x01, NULL);
	CHECK_STATUS(status, -3, cleanup);
	testResult = UnitTest::RunAllTests();
	CHECK_STATUS(testResult, testResult, cleanup);
cleanup:
	flClose(g_handle);
	return retVal;
}
