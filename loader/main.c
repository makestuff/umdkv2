/*
 * Copyright (C) 2009-2012 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <libfpgalink.h>
#include "args.h"

int main(int argc, const char *argv[]) {
	int retVal;
	struct FLContext *handle = NULL;
	FLStatus status;
	const char *error = NULL;
	uint8 byte = 0x10;
	uint8 flag;
	size_t numBytes, numWords, i, numSame;
	uint8 *fileBuffer = NULL;
	uint8 *readBuffer = NULL;
	const char *vp = NULL, *ivp = NULL, *progConfig = NULL, *dataFile = NULL;
	const char *const prog = argv[0];
	uint8 command[8];
	bool compare = false;

	printf("FPGALink \"C\" Example Copyright (C) 2011-2013 Chris McClelland\n\n");
	argv++;
	argc--;
	while ( argc ) {
		if ( argv[0][0] != '-' ) {
			unexpected(prog, *argv);
			FAIL(1, cleanup);
		}
		switch ( argv[0][1] ) {
		case 'h':
			usage(prog);
			FAIL(0, cleanup);
			break;
		case 'c':
			compare = true;
			break;
		case 'v':
			GET_ARG("v", vp, 4, cleanup);
			break;
		case 'i':
			GET_ARG("i", ivp, 5, cleanup);
			break;
		case 'p':
			GET_ARG("p", progConfig, 6, cleanup);
			break;
		case 'f':
			GET_ARG("f", dataFile, 7, cleanup);
			break;
		default:
			invalid(prog, argv[0][1]);
			FAIL(8, cleanup);
		}
		argv++;
		argc--;
	}
	if ( !vp ) {
		missing(prog, "v <VID:PID>");
		FAIL(9, cleanup);
	}

	status = flInitialise(0, &error);
	CHECK_STATUS(status, 10, cleanup);
	
	printf("Checking for presence of FPGALink device %s...\n", vp);
	status = flIsDeviceAvailable(vp, &flag, &error);
	CHECK_STATUS(status, 12, cleanup);
	if ( !flag ) {
		if ( ivp ) {
			int count = 60;
			printf("FPGALink device not found; loading firmware into %s...\n", ivp);
			status = flLoadStandardFirmware(ivp, vp, &error);
			CHECK_STATUS(status, 11, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				status = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(status, 12, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(13, cleanup);
			}
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(15, cleanup);
		}
	}
	printf("Attempting to open connection to FPGLink device %s...\n", vp);
	status = flOpen(vp, &handle, &error);
	CHECK_STATUS(status, 14, cleanup);
	
	if ( progConfig ) {
		printf("Executing programming configuration \"%s\" on FPGALink device %s...\n", progConfig, vp);
		status = flProgram(handle, progConfig, NULL, &error);
		CHECK_STATUS(status, 19, cleanup);
	}
	
	if ( dataFile ) {
		printf("Loading %s...\n", dataFile);
		fileBuffer = flLoadFile(dataFile, &numBytes);
		if ( !fileBuffer ) {
			fprintf(stderr, "Unable to load file %s!\n", dataFile);
			FAIL(25, cleanup);
		}
		if ( numBytes & 1 ) {
			fprintf(stderr, "File %s contains an odd number of bytes!\b", dataFile);
			FAIL(26, cleanup);
		}
		
		status = flSelectConduit(handle, 0x01, &error);
		CHECK_STATUS(status, 21, cleanup);
		
		if ( !compare ) {
			printf("Putting MD in reset...\n");
			byte = 0x01;
			status = flWriteChannel(handle, 0x01, 1, &byte, &error);
			CHECK_STATUS(status, 22, cleanup);
		}
		
		numWords = numBytes / 2;
		command[0] = command[1] = command[2] = command[3] = 0x00;
		command[7] = (uint8)numWords;
		numWords >>= 8;
		command[6] = (uint8)numWords;
		numWords >>= 8;
		command[5] = (uint8)numWords;
		if ( compare ) {
			printf("Comparing ROM...\n");
			command[4] = 0x40;
			status = flWriteChannel(handle, 0x00, 8, command, &error);
			CHECK_STATUS(status, 22, cleanup);
			readBuffer = malloc(numBytes);
			if ( !readBuffer ) {
				fprintf(stderr, "Unable to load file %s!\n", dataFile);
				FAIL(25, cleanup);
			}
			status = flReadChannel(handle, 0x00, numBytes, readBuffer, &error);
			CHECK_STATUS(status, 22, cleanup);
			
			numSame = 0;
			for ( i = 0; i < numBytes; i++ ) {
				if ( readBuffer[i] == fileBuffer[i] ) {
					numSame++;
				}
			}
			printf("Memory is %0.5f%% identical\n", 100.0 * (double)numSame/(double)numBytes);
			if ( numSame != numBytes ) {
				FILE *dumpFile = fopen("out.dat", "wb");
				size_t numWritten;
				if ( !dumpFile ) {
					fprintf(stderr, "Dump file cannot be written!\n");
					FAIL(26, cleanup);
				}
				numWritten = fwrite(readBuffer, 1, numBytes, dumpFile);
				if ( numWritten != numBytes ) {
					fprintf(stderr, "Wrote only %zu bytes to dump file (expected to write %zu)!\n", numWritten, numBytes);
					FAIL(26, cleanup);
				}
				printf("Diffs found, so I saved the readback data to out.dat\n");
			}
			
		} else {
			printf("Writing ROM...\n");
			command[4] = 0x80;
			status = flWriteChannel(handle, 0x00, 8, command, &error);
			CHECK_STATUS(status, 22, cleanup);
			status = flWriteChannel(handle, 0x00, numBytes, fileBuffer, &error);
			CHECK_STATUS(status, 22, cleanup);
			
			printf("Releasing MD from reset...\n");
			byte = 0x00;
			status = flWriteChannel(handle, 0x01, 1, &byte, &error);
			CHECK_STATUS(status, 22, cleanup);
		}
	}
	retVal = 0;
cleanup:
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	free(readBuffer);
	flFreeFile(fileBuffer);
	flClose(handle);
	return retVal;
}

void usage(const char *prog) {
	printf("Usage: %s [-ch] [-i <VID:PID>] -v <VID:PID> [-p <progConfig>]\n", prog);
	printf("          [-f <dataFile>]\n\n");
	printf("Load FX2LP firmware, load the FPGA, interact with the FPGA.\n\n");
	printf("  -i <VID:PID>    initial vendor and product ID of the FPGALink device\n");
	printf("  -v <VID:PID>    renumerated vendor and product ID of the FPGALink device\n");
	printf("  -p <progConfig> configuration and programming file\n");
	printf("  -f <dataFile>   ROM to load\n");
	printf("  -c              compare what's in memory with what's in the file\n");
	printf("  -h              print this help and exit\n");
}
