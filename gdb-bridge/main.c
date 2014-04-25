#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <liberror.h>
#include "remote.h"
#include "escape.h"
#include "mem.h"
#include "args.h"

void setDebug(bool);

static int readMessage(int conn, char *buf, int bufSize) {
	char *ptr = buf;
	const char *const bufEnd = buf + bufSize;
	char dummy[2], ch;
	do {
		if ( read(conn, &ch, 1) <= 0 ) {
			return -1;
		}
		*ptr++ = ch;
	} while ( ptr < bufEnd && ch != '#' );
	if ( ptr == bufEnd && ch != '#' ) {
		return -3;
	}
	if ( read(conn, dummy, 2) != 2 ) {
		return -4;
	}
	return ptr - buf - 1;
}

static void setNonBlocking(int conn, bool nonBlocking) {
    int flags = fcntl(conn, F_GETFL, 0);
	if ( flags == -1 ) {
		flags = 0;
	}
	if ( nonBlocking ) {
		fcntl(conn, F_SETFL, flags | O_NONBLOCK);
	} else {
		fcntl(conn, F_SETFL, flags & ~O_NONBLOCK);
	}
}

/*static bool isInterrupted(const struct I68K *cpu) {
	char ch;
	int numRead = read(cpu->conn, &ch, 1);
	if ( numRead == 1 && ch == 0x03 ) {
		return true;
	} else {
		return false;
	}
}

void printMessage(const unsigned char *data, int length) {
	while ( length-- ) {
		printf("%02X", *data++);
	}
	printf("\n");
}*/

static int handleConnection(int conn, struct FLContext *handle) {
	char buffer[SOCKET_BUFFER_SIZE];
	int bytesRead;
	setNonBlocking(conn, false);
	bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	while ( bytesRead > 0 ) {
		buffer[bytesRead] = 0;
		//printf("msg: ");printMessage((const unsigned char *)buffer, bytesRead);
		setNonBlocking(conn, true);
		processMessage(buffer, bytesRead, conn, handle);
		setNonBlocking(conn, false);
		bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	}
	return bytesRead;
}

void usage(const char *prog) {
	printf("Usage: %s [-crd] [-w <file:addr>] [-l <listenPort>] [-b <brkAddr>]\n\n", prog);
	printf("Interact with the UMDKv2 cartridge.\n\n");
	printf("  -w <file:addr>   write the file to the given address\n");
	printf("  -l <listenPort>  listen for GDB connections on the given port\n");
	printf("  -b <brkAddr>     address to use to interrupt execution\n");
	printf("  -c               continue execution\n");
	printf("  -r               simulate a reset\n");
	printf("  -d               enable GDB RAM dumps & tracing\n");
	printf("  -h               print this help and exit\n");
}

int main(int argc, char *argv[]) {
	int retVal = 0;
	int server = -1;
	int conn = -1;
	socklen_t clientAddrLen;
	struct sockaddr_in serverAddress = {0,};
	struct sockaddr_in clientAddress = {0,};
	union {
		uint32 ip4;
		char ip[4];
	} u;
	const char *error = NULL;
	FLStatus fStatus;
	int uStatus;
	struct FLContext *handle = NULL;
	bool doCont = false, doReset = false, doDebug = false;
	const char *wrFile = NULL, *listenPortStr = NULL, *brkAddrStr = NULL;
	char *loadFile = NULL;
	uint8 *loadData = NULL;
	uint16 listenPort = 0;
	uint32 brkAddr = 0, loadAddr = 0, loadSize = 0;
	const char *const prog = argv[0];
	printf("UMDKv2 Bridge Tool Copyright (C) 2014 Chris McClelland\n\n");
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
		case 'w':
			GET_ARG("w", wrFile, 7, cleanup);
			break;
		case 'l':
			GET_ARG("l", listenPortStr, 6, cleanup);
			break;
		case 'b':
			GET_ARG("r", brkAddrStr, 7, cleanup);
			break;
		case 'c':
			doCont = true;
			break;
		case 'r':
			doReset = true;
			break;
		case 'd':
			doDebug = true;
			break;
		default:
			invalid(prog, argv[0][1]);
			FAIL(2, cleanup);
		}
		argv++;
		argc--;
	}
	if ( wrFile ) {
		size_t fileNameLength, numBytes;
		const char *ptr = wrFile;
		char ch = *ptr;
		while ( ch && ch != ':' ) {
			ch = *++ptr;
		}
		if ( ch ) {
			if ( ch != ':' ) {
				fprintf(stderr, "Invalid argument to option -w <file:addr>\n");
				FAIL(1, cleanup);
			}
			fileNameLength = ptr - wrFile;
			ptr++;
			loadAddr = (uint32)strtoul(ptr, (char**)&ptr, 0);
			if ( *ptr != '\0' ) {
				fprintf(stderr, "Invalid argument to option -w <file:addr>\n");
				FAIL(15, cleanup);
			}
			loadFile = malloc(fileNameLength + 1);
			if ( !loadFile ) {
				fprintf(stderr, "Memory allocation error!\n");
				FAIL(14, cleanup);
			}
			strncpy(loadFile, wrFile, fileNameLength);
			loadFile[fileNameLength] = '\0';
			loadData = flLoadFile(loadFile, &numBytes);
			if ( !loadData ) {
				fprintf(stderr, "Unable to load file %s!\n", loadFile);
				FAIL(14, cleanup);
			}
			if ( numBytes & 1 ) {
				fprintf(stderr, "File %s contains an odd number of bytes!\n", loadFile);
				FAIL(15, cleanup);
			}
			free(loadFile);
			loadFile = NULL;
		} else {
			loadData = flLoadFile(wrFile, &numBytes);
			if ( !loadData ) {
				fprintf(stderr, "Unable to load file %s!\n", wrFile);
				FAIL(14, cleanup);
			}
			if ( numBytes & 1 ) {
				fprintf(stderr, "File %s contains an odd number of bytes!\n", wrFile);
				FAIL(15, cleanup);
			}
		}
		printf("Got 0x%06X bytes to load at 0x%06X\n", (uint32)numBytes, loadAddr);
		loadSize = (uint32)numBytes;
	}
	if ( listenPortStr ) {
		const char *ptr = listenPortStr;
		listenPort = (uint16)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != '\0' ) {
			fprintf(stderr, "Invalid argument to option -l <listenPort>\n");
			FAIL(15, cleanup);
		}
		printf("listenPort = %d\n", listenPort);
	}
	if ( brkAddrStr ) {
		const char *ptr = brkAddrStr;
		brkAddr = (uint32)strtoul(ptr, (char**)&ptr, 0);
		if ( *ptr != '\0' ) {
			fprintf(stderr, "Invalid argument to option -b <brkAddr>\n");
			FAIL(15, cleanup);
		}
		printf("brkAddr = 0x%06X\n", brkAddr);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, 1, cleanup);

	fStatus = flOpen("1d50:602b", &handle, NULL);
	CHECK_STATUS(fStatus, 1, cleanup);

	fStatus = flSelectConduit(handle, 0x01, NULL);
	CHECK_STATUS(fStatus, 1, cleanup);

	// Maybe enable debugging
	setDebug(doDebug);

	// Maybe load some data
	if ( loadData ) {
		uint16 cmdFlag, oldOp;
		uint32 vbAddr;
		uStatus = umdkDirectReadWord(handle, CB_FLAG, &cmdFlag, NULL);
		CHECK_STATUS(uStatus, uStatus, cleanup);
		if ( cmdFlag != CF_READY ) {
			// Read address of VDP vertical interrupt vector & read 1st opcode
			uStatus = umdkDirectReadLong(handle, VB_VEC, &vbAddr, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
			uStatus = umdkDirectReadWord(handle, vbAddr, &oldOp, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
			printf("vbAddr = 0x%06X, opCode = 0x%04X\n", vbAddr, oldOp);
			
			// Replace illegal instruction vector
			uStatus = umdkDirectWriteLong(handle, IL_VEC, MONITOR, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
			
			// Write illegal instruction opcode
			uStatus = umdkDirectWriteWord(handle, vbAddr, ILLEGAL, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
			
			// Acquire the monitor
			uStatus = umdkRemoteAcquire(handle, NULL, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
			
			// Restore old opcode to vbAddr
			uStatus = umdkDirectWriteWord(handle, vbAddr, oldOp, NULL);
			CHECK_STATUS(uStatus, uStatus, cleanup);
		}
		uStatus = umdkPhysicalWriteBytes(handle, loadAddr, loadSize, loadData, NULL);
		CHECK_STATUS(uStatus, uStatus, cleanup);
	}
	if ( doReset ) {
		uStatus = umdkReset(handle, NULL);
		CHECK_STATUS(uStatus, uStatus, cleanup);
	} else if ( doCont ) {
		uStatus = umdkContinue(handle, NULL);
		CHECK_STATUS(uStatus, uStatus, cleanup);
	}
	if ( listenPortStr ) {
		server = socket(AF_INET, SOCK_STREAM, 0);
		if ( server < 0 ) {
			errRenderStd(&error);
			FAIL(1, cleanup);
		}
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(listenPort);
		retVal = bind(server, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
		if ( retVal < 0 ) {
			errRenderStd(&error);
			FAIL(2, cleanup);
		}
		for ( ; ; ) {
			printf("Waiting for GDB connection on :%d...\n", listenPort);
			listen(server, 5);
			clientAddrLen = sizeof(clientAddress);
			conn = accept(server, (struct sockaddr *)&clientAddress, &clientAddrLen);
			if ( conn < 0 ) {
				errRenderStd(&error);
				FAIL(3, cleanup);
			}
			u.ip4 = clientAddress.sin_addr.s_addr;
			printf("Got GDB connection from %d.%d.%d.%d:%d\n", u.ip[0], u.ip[1], u.ip[2], u.ip[3], clientAddress.sin_port);
			setConnection(conn);
			handleConnection(conn, handle);
			printf("GDB disconnected\n");
		}
	}
cleanup:
	if ( loadFile ) {
		free(loadFile);
	}
	if ( loadData ) {
		flFreeFile(loadData);
	}
	if ( error ) {
		fprintf(stderr, "%s: %s\n", argv[0], error);
		flFreeError(error);
	}
	if ( conn >= 0 ) {
		close(conn);
	}
	if ( server >= 0 ) {
		close(server);
	}
	if ( handle ) {
		flClose(handle);
	}
	return retVal;
}
