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
#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <makestuff.h>
#include <libbuffer.h>
#include <liberror.h>

void dumpCode(const char *name, const struct Buffer *buf) {
	if ( buf->length > 0 ) {
		const uint8 *ptr;
		const uint8 *end;
		uint8 count;
		printf("const unsigned int %sCodeSize = %lu;\nconst unsigned char %sCodeData[] = {\n", name, buf->length, name);
		ptr = buf->data;
		end = buf->data + buf->length;
		count = 1;
		printf("\t0x%02X", *ptr++);
		while ( ptr < end ) {
			if ( count == 0 ) {
				printf(",\n\t0x%02X", *ptr++);
			} else {
				printf(", 0x%02X", *ptr++);
			}
			count++;
			count &= 15;
		}
		printf("\n};\n");
	}
}

int main(int argc, const char *argv[]) {
	int returnCode;
	struct Buffer data = {0,};
	BufferStatus status;
	const char *error = NULL;

	if ( argc != 3 ) {
		fprintf(stderr, "Synopsis: %s <monitor.bin> <name>\n", argv[0]);
		FAIL(1);
	}

	status = bufInitialise(&data, 0x4000, 0x00, &error);
	if ( status ) {
		FAIL(2);
	}

	status = bufAppendFromBinaryFile(&data, argv[1], &error);
	if ( status ) {
		FAIL(2);
	}

	dumpCode(argv[2], &data);

	returnCode = 0;

cleanup:
	if ( data.data ) {
		bufDestroy(&data);
	}
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		errFree(error);
	}
	return returnCode;
}
