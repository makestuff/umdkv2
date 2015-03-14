#include <stdio.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <io.h>
	#define read _read
#else
	#include <unistd.h>
	#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include "escape.h"

static SOCKET g_conn;

void setNonBlocking(SOCKET conn, bool nonBlocking) {
	#ifdef WIN32
		unsigned long mode = nonBlocking ? 1 : 0;
		ioctlsocket(conn, FIONBIO, &mode);
	#else
		int flags = fcntl(conn, F_GETFL, 0);
		if ( flags == -1 ) {
			flags = 0;
		}
		if ( nonBlocking ) {
			fcntl(conn, F_SETFL, flags | O_NONBLOCK);
		} else {
			fcntl(conn, F_SETFL, flags & ~O_NONBLOCK);
		}
	#endif
}

void setConnection(SOCKET conn) {
	g_conn = conn;
}

bool isInterrupted(void) {
	uint8 buf = 0x03;
	int status;
	setNonBlocking(g_conn, true);
	status = recv(g_conn, &buf, 1, 0);
	setNonBlocking(g_conn, false);
	if ( status == 0 ) {
		//printf("socket closed\n");
		return true; // socket closed
	} else if ( status == -1 ) {
		//const int x = WSAGetLastError();
		//if ( errno != x ) {
		//	printf("x = %d\n", x);
		//}
		if ( errno == EAGAIN || errno == EWOULDBLOCK || errno == 0 ) {
			//printf("errno == EAGAIN || errno == EWOULDBLOCK\n");
			return false;
		} else {
			//perror("other error: ");
			return true; // other errors
		}
	} else {
		if ( buf == 0x03 ) {
			//printf("buf == 0x03\n");
			return true;
		} else {
			//printf("buf != 0x03\n");
			return false;
		}
	}
}	
