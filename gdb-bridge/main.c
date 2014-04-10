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
}*/

static int handleConnection(int conn, struct FLContext *handle) {
	char buffer[SOCKET_BUFFER_SIZE];
	int bytesRead;
	setNonBlocking(conn, false);
	bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	while ( bytesRead > 0 ) {
		buffer[bytesRead] = 0;
		setNonBlocking(conn, true);
		processMessage(buffer, bytesRead, conn, handle);
		setNonBlocking(conn, false);
		bytesRead = readMessage(conn, buffer, SOCKET_BUFFER_SIZE);
	}
	return bytesRead;
}

int main(int argc, char *argv[]) {
	int retVal = 0;
	int server = -1;
	int conn = -1;
	uint16 port;
	socklen_t clientAddrLen;
	struct sockaddr_in serverAddress = {0,};
	struct sockaddr_in clientAddress = {0,};
	union {
		uint32 ip4;
		char ip[4];
	} u;
	const char *error = NULL;
	FLStatus fStatus;
	struct FLContext *handle = NULL;
	if ( argc != 2 ) {
		fprintf(stderr, "Synopsis: %s <port>\n", argv[0]);
		FAIL(1, cleanup);
	}
	port = (uint16)strtoul(argv[1], NULL, 0);
	if ( !port ) {
		fprintf(stderr, "Invalid port!\n");
		FAIL(2, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, 1, cleanup);

	fStatus = flOpen("1d50:602b", &handle, NULL);
	CHECK_STATUS(fStatus, 1, cleanup);

	fStatus = flSelectConduit(handle, 0x01, NULL);
	CHECK_STATUS(fStatus, 1, cleanup);

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
		setConnection(conn);
		handleConnection(conn, handle);
		printf("GDB disconnected\n");
	}
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
	if ( handle ) {
		flClose(handle);
	}
	return retVal;
}
