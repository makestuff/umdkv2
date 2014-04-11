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
#include <string.h>
#include <libfpgalink.h>
#include "args.h"

bool sigIsRaised(void);
void sigRegisterHandler(void);

int main(int argc, const char *argv[]) {
	int retVal = 0;
	struct FLContext *handle = NULL;
	FLStatus status;
	const char *error = NULL;
	uint8 byte = 0x10;
	uint8 flag;
	size_t i;
	uint8 *writeBuf = NULL;
	uint8 *readBuf = NULL;
	const char *vp = "1d50:602b", *ivp = NULL, *progConfig = NULL;
	const char *const prog = argv[0];
	uint8 command[10];
	const char *execCtrl = NULL, *execTrace = NULL;
	const char *rdFile = NULL, *wrFile = NULL, *cmpFile = NULL;
	size_t fileNameLength;
	char *filePart = NULL;
	FILE *outFile = NULL;

	printf("UMDKv2 Loader Copyright (C) 2014 Chris McClelland\n\n");
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
		case 'i':
			GET_ARG("i", ivp, 5, cleanup);
			break;
		case 'v':
			GET_ARG("v", vp, 4, cleanup);
			break;
		case 'p':
			GET_ARG("p", progConfig, 6, cleanup);
			break;
		case 'w':
			GET_ARG("w", wrFile, 7, cleanup);
			break;
		case 'r':
			GET_ARG("r", rdFile, 7, cleanup);
			break;
		case 'c':
			GET_ARG("c", cmpFile, 7, cleanup);
			break;
		case 'x':
			GET_ARG("x", execCtrl, 7, cleanup);
			break;
		case 't':
			GET_ARG("t", execTrace, 8, cleanup);
			break;
		default:
			invalid(prog, argv[0][1]);
			FAIL(2, cleanup);
		}
		argv++;
		argc--;
	}
	status = flInitialise(0, &error);
	CHECK_STATUS(status, 6, cleanup);
	
	status = flIsDeviceAvailable(vp, &flag, &error);
	CHECK_STATUS(status, 7, cleanup);
	if ( !flag ) {
		if ( ivp ) {
			int count = 60;
			printf("FPGALink device not found; loading firmware into %s...\n", ivp);
			status = flLoadStandardFirmware(ivp, vp, &error);
			CHECK_STATUS(status, 8, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				status = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(status, 9, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(10, cleanup);
			}
		} else {
			fprintf(stderr, "Could not find any %s devices and no initial VID:PID was supplied\n", vp);
			FAIL(11, cleanup);
		}
	}
	status = flOpen(vp, &handle, &error);
	CHECK_STATUS(status, 12, cleanup);

	if ( progConfig ) {
		printf("Executing programming configuration \"%s\" on FPGALink device %s...\n", progConfig, vp);
		status = flProgram(handle, progConfig, NULL, &error);
		CHECK_STATUS(status, 13, cleanup);
	}

	status = flSelectConduit(handle, 0x01, &error);
	CHECK_STATUS(status, 16, cleanup);

	i = 0;
	if ( rdFile ) { i++; }
	if ( wrFile ) { i++; }
	if ( cmpFile ) { i++; }
	if ( i > 1 ) {
		fprintf(stderr, "The -r, -w and -c options are mutually exclusive\n");
		FAIL(2, cleanup);
	}

	if ( execCtrl ) {
		if ( execCtrl[1] != '\0' || execCtrl[0] < '0' || execCtrl[0] > '2' ) {
			fprintf(stderr, "Invalid argument to option -x <0|1|2>\n");
			FAIL(1, cleanup);
		}
		if ( execCtrl[0] != 1 ) {
			printf("Putting MD in reset...\n");
			byte = 0x01;
			status = flWriteChannel(handle, 0x01, 1, &byte, &error);
			CHECK_STATUS(status, 17, cleanup);
		}
	}

	if ( wrFile ) {
		size_t address = 0;
		size_t numBytes, numWords;
		const char *ptr = wrFile;
		char ch = *ptr;
		while ( ch && ch != ':' ) {
			ch = *++ptr;
		}
		if ( ch != ':' ) {
			fprintf(stderr, "Invalid argument to option -w <file:addr>\n");
			FAIL(1, cleanup);
		}
		fileNameLength = ptr - wrFile;
		ptr++;
		address = (size_t)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != '\0' ) {
			fprintf(stderr, "Invalid argument to option -w <file:addr>\n");
			FAIL(15, cleanup);
		}
		if ( address & 1 ) {
			fprintf(stderr, "Address cannot be an odd number!\n");
			FAIL(16, cleanup);
		}

		filePart = malloc(fileNameLength + 1);
		if ( !filePart ) {
			fprintf(stderr, "Memory allocation error!\n");
			FAIL(14, cleanup);
		}
		strncpy(filePart, wrFile, fileNameLength);
		filePart[fileNameLength] = '\0';
		writeBuf = flLoadFile(filePart, &numBytes);
		if ( !writeBuf ) {
			fprintf(stderr, "Unable to load file %s!\n", filePart);
			FAIL(14, cleanup);
		}
		if ( numBytes & 1 ) {
			fprintf(stderr, "File %s contains an odd number of bytes!\n", filePart);
			FAIL(15, cleanup);
		}

		numWords = numBytes / 2;
		address = address / 2;

		// Now send actual data
		command[0] = 0x00; // set mem-pipe pointer
		command[3] = (uint8)address;
		address >>= 8;
		command[2] = (uint8)address;
		address >>= 8;
		command[1] = (uint8)address;
		command[4] = 0x80; // write words to SDRAM
		command[7] = (uint8)numWords;
		numWords >>= 8;
		command[6] = (uint8)numWords;
		numWords >>= 8;
		command[5] = (uint8)numWords;
		
		printf("Writing SDRAM...\n");
		status = flWriteChannelAsync(handle, 0x00, 8, command, &error);
		CHECK_STATUS(status, 23, cleanup);
		status = flWriteChannel(handle, 0x00, numBytes, writeBuf, &error);
		CHECK_STATUS(status, 24, cleanup);
	}

	if ( rdFile ) {
		size_t address;
		size_t numBytes, numWords, numWritten;
		const char *ptr = rdFile;
		char ch = *ptr;
		while ( ch && ch != ':' ) {
			ch = *++ptr;
		}
		if ( ch != ':' ) {
			fprintf(stderr, "Invalid argument to option -r <file:addr:len>\n");
			FAIL(15, cleanup);
		}
		fileNameLength = ptr - rdFile;
		ptr++;
		address = (size_t)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != ':' ) {
			fprintf(stderr, "Invalid argument to option -r <file:addr:len>\n");
			FAIL(15, cleanup);
		}
		if ( address & 1 ) {
			fprintf(stderr, "Address cannot be an odd number!\n");
			FAIL(16, cleanup);
		}
		ptr++;
		numBytes = (size_t)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != '\0' ) {
			fprintf(stderr, "Invalid argument to option -r <file:addr:len>\n");
			FAIL(15, cleanup);
		}
		if ( numBytes & 1 ) {
			fprintf(stderr, "Length cannot be an odd number!\n");
			FAIL(16, cleanup);
		}

		filePart = malloc(fileNameLength + 1);
		if ( !filePart ) {
			fprintf(stderr, "Memory allocation error!\n");
			FAIL(14, cleanup);
		}
		strncpy(filePart, rdFile, fileNameLength);
		filePart[fileNameLength] = '\0';
		readBuf = malloc(numBytes);
		if ( !readBuf ) {
			fprintf(stderr, "Memory allocation error!\n");
			FAIL(15, cleanup);
		}

		numWords = numBytes / 2;
		address = address / 2;

		command[0] = 0x00; // set mem-pipe pointer
		command[3] = (uint8)address;
		address >>= 8;
		command[2] = (uint8)address;
		address >>= 8;
		command[1] = (uint8)address;
		
		command[4] = 0x40; // read words to SDRAM
		command[7] = (uint8)numWords;
		numWords >>= 8;
		command[6] = (uint8)numWords;
		numWords >>= 8;
		command[5] = (uint8)numWords;

		outFile = fopen(filePart, "wb");
		if ( !outFile ) {
			fprintf(stderr, "%s cannot be opened for writing!\n", filePart);
			FAIL(19,cleanup);
		}

		printf("Reading SDRAM...\n");
		status = flWriteChannelAsync(handle, 0x00, 8, command, &error);
		CHECK_STATUS(status, 18, cleanup);
		status = flReadChannel(handle, 0x00, numBytes, readBuf, &error);
		CHECK_STATUS(status, 20, cleanup);
		numWritten = fwrite(readBuf, 1, numBytes, outFile);
		if ( numWritten != numBytes ) {
			fprintf(stderr, "Wrote only %zu bytes to dump file (expected to write %zu)!\n", numWritten, numBytes);
			FAIL(22, cleanup);
		}
	}

	if ( cmpFile ) {
		size_t address = 0;
		size_t numBytes, numWords, numSame, numWritten;
		const char *ptr = cmpFile;
		char ch = *ptr;
		while ( ch && ch != ':' ) {
			ch = *++ptr;
		}
		if ( ch != ':' ) {
			fprintf(stderr, "Invalid argument to option -c <file:addr>\n");
			FAIL(1, cleanup);
		}
		fileNameLength = ptr - cmpFile;
		ptr++;
		address = (size_t)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != '\0' ) {
			fprintf(stderr, "Invalid argument to option -c <file:addr>\n");
			FAIL(15, cleanup);
		}
		if ( address & 1 ) {
			fprintf(stderr, "Address cannot be an odd number!\n");
			FAIL(16, cleanup);
		}

		filePart = malloc(fileNameLength + 1);
		if ( !filePart ) {
			fprintf(stderr, "Memory allocation error!\n");
			FAIL(14, cleanup);
		}
		strncpy(filePart, cmpFile, fileNameLength);
		filePart[fileNameLength] = '\0';
		writeBuf = flLoadFile(filePart, &numBytes);
		if ( !writeBuf ) {
			fprintf(stderr, "Unable to load file %s!\n", filePart);
			FAIL(14, cleanup);
		}
		if ( numBytes & 1 ) {
			fprintf(stderr, "File %s contains an odd number of bytes!\n", filePart);
			FAIL(15, cleanup);
		}

		readBuf = malloc(numBytes);
		if ( !readBuf ) {
			fprintf(stderr, "Memory allocation error!\n");
			FAIL(15, cleanup);
		}

		numWords = numBytes / 2;
		address = address / 2;

		command[0] = 0x00; // set mem-pipe pointer
		command[3] = (uint8)address;
		address >>= 8;
		command[2] = (uint8)address;
		address >>= 8;
		command[1] = (uint8)address;
		
		command[4] = 0x40; // read words to SDRAM
		command[7] = (uint8)numWords;
		numWords >>= 8;
		command[6] = (uint8)numWords;
		numWords >>= 8;
		command[5] = (uint8)numWords;

		printf("Comparing SDRAM contents with file...\n");
		status = flWriteChannelAsync(handle, 0x00, 8, command, &error);
		CHECK_STATUS(status, 18, cleanup);
		status = flReadChannel(handle, 0x00, numBytes, readBuf, &error);
		CHECK_STATUS(status, 20, cleanup);

		numSame = 0;
		for ( i = 0; i < numBytes; i++ ) {
			if ( readBuf[i] == writeBuf[i] ) {
				numSame++;
			}
		}
		if ( numSame != numBytes ) {
			printf("Memory is only %0.5f%% identical: writing readback to dump.bin!\n", 100.0 * (double)numSame/(double)numBytes);
			outFile = fopen("dump.bin", "wb");
			if ( !outFile ) {
				fprintf(stderr, "\nDump file dump.bin cannot be opened for writing!\n");
				FAIL(21, cleanup);
			}
			numWritten = fwrite(readBuf, 1, numBytes, outFile);
			if ( numWritten != numBytes ) {
				fprintf(stderr, "\nWrote only %zu bytes to dump file (expected to write %zu)!\n", numWritten, numBytes);
				FAIL(22, cleanup);
			}
		} else {
			printf("Memory is identical!\n");
		}
	}

	if ( execCtrl && execCtrl[0] != '0' ) {
		// Write CF_RUNNING (0x0000) to CB_FLAG, to show that MD is running
		uint32 flagAddr = 0xF80400/2;
		command[0] = 0x00; // set mem-pipe pointer
		command[3] = (uint8)flagAddr;
		flagAddr >>= 8;
		command[2] = (uint8)flagAddr;
		flagAddr >>= 8;
		command[1] = (uint8)flagAddr;
		command[4] = 0x80; // write words to SDRAM
		command[5] = 0x00;
		command[6] = 0x00;
		command[7] = 0x01; // one 16-bit word
		command[8] = 0x00; // CF_RUNNING
		command[9] = 0x00;
		status = flWriteChannelAsync(handle, 0x00, 10, command, &error);
		CHECK_STATUS(status, 23, cleanup);

		printf("Releasing MD from reset...\n");
		byte = 0x00;
		status = flWriteChannelAsync(handle, 0x01, 1, &byte, &error);
		CHECK_STATUS(status, 25, cleanup);
	}

	if ( execTrace ) {
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		uint8 scrapData[16400];
		size_t scrapSize;
		printf("Dumping execution trace to %s\n", execTrace);
		file = fopen(execTrace, "wb");
		CHECK_STATUS(!file, 26, cleanup);
		sigRegisterHandler();
		status = flSelectConduit(handle, 1, &error);
		CHECK_STATUS(status, 27, cleanup);

		// Disable tracing (if any) & clear junk from trace FIFO
		byte = 0x00;
		status = flWriteChannel(handle, 0x01, 1, &byte, &error);
		CHECK_STATUS(status, 25, cleanup);
		status = flReadChannel(handle, 0x03, 1, &byte, &error);
		CHECK_STATUS(status, 20, cleanup);
		scrapSize = byte << 8;
		status = flReadChannel(handle, 0x04, 1, &byte, &error);
		CHECK_STATUS(status, 20, cleanup);
		scrapSize |= byte;
		//printf("scrapSize = "PFSZD"\n", scrapSize);
		while ( scrapSize ) {
			// Clear junk from FIFO
			status = flReadChannel(handle, 0x02, scrapSize, scrapData, &error);
			CHECK_STATUS(status, 20, cleanup);

			// Verify no junk remaining
			status = flReadChannel(handle, 0x03, 1, &byte, &error);
			CHECK_STATUS(status, 20, cleanup);
			scrapSize = byte << 8;
			status = flReadChannel(handle, 0x04, 1, &byte, &error);
			CHECK_STATUS(status, 20, cleanup);
			scrapSize |= byte;
			//printf("scrapSize = "PFSZD"\n", scrapSize);
		}

		// Enable tracing
		byte = 0x02;
		status = flWriteChannelAsync(handle, 0x01, 1, &byte, &error);
		CHECK_STATUS(status, 25, cleanup);

		// Start reading
		status = flReadChannelAsyncSubmit(handle, 2, 22528, NULL, &error);
		CHECK_STATUS(status, 28, cleanup);
		do {
			status = flReadChannelAsyncSubmit(handle, 2, 22528, NULL, &error);
			CHECK_STATUS(status, 29, cleanup);
			status = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(status, 30, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		status = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(status, 31, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}
cleanup:
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	if ( outFile ) {
		fclose(outFile);
	}
	free(filePart);
	free(readBuf);
	flFreeFile(writeBuf);
	flClose(handle);
	return retVal;
}

void usage(const char *prog) {
	printf("Usage: %s [-crqh] [-i <VID:PID>] -v <VID:PID> [-p <progConfig>]\n", prog);
	printf("          [-r <file:addr>] [-w <file:addr:len>]\n\n");
	printf("Load FX2LP firmware, load the FPGA, interact with the FPGA.\n\n");
	printf("  -i <VID:PID>       initial vendor and product ID of the FPGALink device\n");
	printf("  -v <VID:PID>       renumerated vendor and product ID of the FPGALink device\n");
	printf("  -p <progConfig>    configuration and programming file\n");
	printf("  -w <file:addr>     write data at the given address\n");
	printf("  -r <file:addr:len> read data from the given address\n");
	printf("  -c <file:addr>     compare the file with what's at the given address\n");
	printf("  -x <0|1|2>         choose 0->reset, 1->execute, 2->reset then execute\n");
	printf("  -t <traceFile>     save execution trace\n");
	printf("  -h                 print this help and exit\n");
}
