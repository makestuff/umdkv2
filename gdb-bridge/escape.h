#ifndef ESCAPE_H
#define ESCAPE_H

#include <makestuff.h>
#ifdef WIN32
	#include <winsock2.h>
#else
	#define SOCKET int
#endif

void setConnection(SOCKET conn);
bool isInterrupted(void);
void setNonBlocking(SOCKET conn, bool nonBlocking);

#endif
