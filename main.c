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
#include <argtable2.h>
#include "remote.h"
#include "emul.h"
#include "umdk.h"
#include "i68k.h"

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

static bool isInterrupted(const struct I68K *cpu) {
	char ch;
	int numRead = read(cpu->conn, &ch, 1);
	if ( numRead == 1 && ch == 0x03 ) {
		return true;
	} else {
		return false;
	}
}

static int handleConnection(int conn, struct I68K *cpu) {
	char buffer[SOCKET_BUFFER_SIZE];
	int bytesRead;
	cpu->conn = conn;
	setNonBlocking(conn, false);
	bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	while ( bytesRead > 0 ) {
		buffer[bytesRead] = 0;
		setNonBlocking(conn, true);
		processMessage(buffer, bytesRead, conn, cpu);
		setNonBlocking(conn, false);
		bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	}
	return bytesRead;
}

#define UMDK

int main(int argc, char *argv[]) {
	struct arg_str *tgtOpt = arg_str1("t", "target", "<star|umdk>", "which backend to connect");
	struct arg_uint *portOpt = arg_uint0("p", "port", "<portNum>", "    which port to listen on for GDB connections");
	struct arg_file *romOpt = arg_file0("r", "rom", "<romFile>", "     ROM file to load");
	struct arg_lit  *startOpt = arg_lit0("s", "start", "             start the ROM running");
	struct arg_uint *brkOpt = arg_uint0("b", "break", "<brkAddr>", "   the address to break at on GDB ctrl-C");
	struct arg_str *vpOpt = arg_str0("v", "vp", "<VID:PID>", "      UMDK: the VID:PID of the device");
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<iVID:iPID>", "   UMDK: the initial VID:PID of the device");
	struct arg_file *xsvfOpt = arg_file0("x", "xsvf", "<xsvfFile>", "   UMDK: XSVF file to play on startup");
	struct arg_lit  *helpOpt = arg_lit0("h", "help", "              print this help and exit\n");
	struct arg_end  *endOpt  = arg_end(20);
	void* argTable[] = {tgtOpt, portOpt, romOpt, startOpt, brkOpt, vpOpt, ivpOpt, xsvfOpt, helpOpt, endOpt};
	const char *progName = "umdk";
	int numErrors;
	int server = -1;
	int conn = -1;
	uint16 port;
	int retVal;
	socklen_t clientAddrLen;
	struct sockaddr_in serverAddress = {0,};
	struct sockaddr_in clientAddress = {0,};
	union {
		uint32 ip4;
		char ip[4];
	} u;
	struct I68K *cpu = NULL;
	const char *error = NULL;
	const char *rom = NULL;
	bool start = false;
	uint32 brkAddr = 0x000000;
	FLStatus fStatus;

	if ( arg_nullcheck(argTable) != 0 ) {
		errRender(&error, "Insufficient memory!");
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("UMDKv2 GDB Daemon Copyright (C) 2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with MegaDrive.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(0, cleanup);
	}

	if ( numErrors ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.", progName);
		FAIL(2, cleanup);
	}

	if ( romOpt->count ) {
		rom = romOpt->filename[0];
	}
	if ( startOpt->count ) {
		start = true;
	}
	if ( brkOpt->count ) {
		brkAddr = brkOpt->ival[0];
	}

	if ( !strcmp(tgtOpt->sval[0], "star") ) {
		cpu = emulCreate();  // TODO: pass in rom to load
	} else if ( !strcmp(tgtOpt->sval[0], "umdk") ) {
		const char *ivp = NULL;
		const char *xsvf = NULL;
		if ( !vpOpt->count ) {
			fprintf(stderr, "With --target=umdk you need to specify --vp=<VID:PID>");
			FAIL(3, cleanup);
		}
		if ( ivpOpt->count ) {
			ivp = ivpOpt->sval[0];
		}
		if ( xsvfOpt->count ) {
			xsvf = xsvfOpt->filename[0];
		}

		fStatus = flInitialise(0, &error);
		CHECK_STATUS(fStatus, 1, cleanup);

		cpu = umdkCreate(vpOpt->sval[0], ivp, xsvf, rom, start, brkAddr, isInterrupted);
		CHECK_STATUS(!cpu, 2, cleanup);
	} else {
		fprintf(stderr, "Unsupported target: %s", tgtOpt->sval[0]);
		FAIL(3, cleanup);
	}

	if ( portOpt->count ) {
		port = (uint16)portOpt->ival[0];
		server = socket(AF_INET, SOCK_STREAM, 0);
		if ( server < 0 ) {
			errRenderStd(&error);
			FAIL(1, cleanup);
		}
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(port);
		retVal = bind(server, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
		if ( retVal < 0 ) {
			errRenderStd(&error);
			FAIL(2, cleanup);
		}
		for ( ; ; ) {
			printf("Waiting for GDB connection on :%d...\n", port);
			listen(server, 5);
			clientAddrLen = sizeof(clientAddress);
			conn = accept(server, (struct sockaddr *)&clientAddress, &clientAddrLen);
			if ( conn < 0 ) {
				errRenderStd(&error);
				FAIL(3, cleanup);
			}
			u.ip4 = clientAddress.sin_addr.s_addr;
			printf("Got GDB connection from %d.%d.%d.%d:%d\n", u.ip[0], u.ip[1], u.ip[2], u.ip[3], clientAddress.sin_port);
			handleConnection(conn, cpu);
			printf("GDB disconnected\n");
		}
	}
	retVal = 0;
cleanup:
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
	if ( cpu ) {
		cpu->vt->destroy(cpu);
	}
	return retVal;
}
