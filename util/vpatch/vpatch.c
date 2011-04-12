/* 
 * Copyright (C) 2009-2011 Chris McClelland
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argtable2.h>
#include <arg_uint.h>
#include <types.h>
#include "read.h"

void dumpSimple(const uint8 *input, uint32 length);
uint32 readLong(const uint8 *p);
void writeLong(uint32 value, uint8 *p);
#define fail(x) exitCode = x; goto cleanup
#define ILLEGAL 0x10
#define TRACE   0x24
#define VBLANK  0x78

int main(int argc, char *argv[]) {

	struct arg_uint *illegalOpt = arg_uint0("i", "illegal", "<address>", "the address of the new illegal instruction handler");
	struct arg_uint *traceOpt   = arg_uint0("t", "trace", "<address>", "  the address of the new trace handler");
	struct arg_uint *vblankOpt  = arg_uint0("v", "vblank", "<address>", " the address of the new vblank handler");
	struct arg_str  *queryOpt   = arg_str0("q", "query", "<i|t|v|l>", "  what to query (illegal, trace, vblank or length)");
	struct arg_lit  *helpOpt    = arg_lit0("h", "help", "             print this help and exit\n");
	struct arg_file *inOpt      = arg_file1(NULL, NULL, "<inFile>", "             the file to read");
	struct arg_file *outOpt     = arg_file0(NULL, NULL, "<outFile>", "             the file to write");
	struct arg_end  *endOpt     = arg_end(20);
	void* argTable[] = {illegalOpt, traceOpt, vblankOpt, queryOpt, helpOpt, inOpt, outOpt, endOpt};
	const char *progName = "vpatch";
	uint32 exitCode = 0;
	int numErrors;
	uint32 vblank, trace, fileLen;
	FILE *outFile = NULL;
	uint8 *buffer = NULL;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		fail(1);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("MegaDrive ROM Vector Patcher Copyright (C) 2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nPatch MegaDrive vectors.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		fail(0);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		fail(2);
	}

	vblank = vblankOpt->count ? vblankOpt->ival[0] : 0UL;
	trace = traceOpt->count ? traceOpt->ival[0] : 0UL;

	buffer = readFile(inOpt->filename[0], &fileLen);
	if ( buffer == NULL ) {
		fprintf(stderr, "Cannot open file %s\n", inOpt->filename[0]);
		fail(3);
	}

	if ( queryOpt->count ) {
		switch ( queryOpt->sval[0][0] ) {
		case 'i':
			printf("0x%08lX\n", readLong(buffer + ILLEGAL));
			break;
		case 't':
			printf("0x%08lX\n", readLong(buffer + TRACE));
			break;
		case 'v':
			printf("0x%08lX\n", readLong(buffer + VBLANK));
			break;
		case 'l':
			printf("0x%08lX\n", fileLen);
			break;
		}
	}

	if ( outOpt->count ) {
		outFile = fopen(outOpt->filename[0], "wb");
		if ( outFile == NULL ) {
			fprintf(stderr, "Cannot open file %s\n", outOpt->filename[0]);
			fail(4);
		}

		if ( illegalOpt->count ) {
			writeLong(illegalOpt->ival[0], buffer + ILLEGAL);
		}
		
		if ( traceOpt->count ) {
			writeLong(traceOpt->ival[0], buffer + TRACE);
		}
		
		if ( vblankOpt->count ) {
			writeLong(vblankOpt->ival[0], buffer + VBLANK);
		}
		
		if ( fwrite(buffer, 1, fileLen, outFile) != fileLen ) {
			fprintf(stderr, "Cannot write to file %s\n", outOpt->filename[0]);
			fail(5);
		}
	}

cleanup:
	if ( buffer ) {
		free(buffer);
	}
	if ( outFile ) {
		fclose(outFile);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));

	return exitCode;
}

void dumpSimple(const uint8 *input, uint32 length) {
	while ( length ) {
		printf(" %02X", *input++);
		--length;
	}
}

uint32 readLong(const uint8 *p) {
	uint32 value = *p++;
	value <<= 8;
    value |= *p++;
    value <<= 8;
    value |= *p++;
    value <<= 8;
    value |= *p;
	return value;
}

void writeLong(uint32 value, uint8 *p) {
	p[3] = value & 0x000000FF;
	value >>= 8;
	p[2] = value & 0x000000FF;
	value >>= 8;
	p[1] = value & 0x000000FF;
	value >>= 8;
	p[0] = value & 0x000000FF;
}
