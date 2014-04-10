#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "escape.h"

static int g_conn;

void setConnection(int conn) {
	g_conn = conn;
}

bool isInterrupted(void) {
	uint8 buf;
	int status;
	int flags = fcntl(g_conn, F_GETFL, 0);
	fcntl(g_conn, F_SETFL, flags | O_NONBLOCK);
	status = read(g_conn, &buf, 1);
	fcntl(g_conn, F_SETFL, flags);
	if ( status == 0 ) {
		return true; // socket closed
	} else if ( status == -1 ) {
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			return false;
		} else {
			return true; // other errors
		}
	} else {
		if ( buf == 0x03 ) {
			return true;
		} else {
			return false;
		}
	}
}	
