#ifndef SOCK_H
#define SOCK_H

#ifdef WIN32
	#include <winsock2.h>
#else
	#define SOCKET int
#endif

#endif
