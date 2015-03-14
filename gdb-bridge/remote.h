#ifndef REMOTE_H
#define REMOTE_H

#include "sock.h"

#define SOCKET_BUFFER_SIZE 1024
struct FLContext;
int processMessage(const char *buf, int size, SOCKET conn, struct FLContext *handle);

#endif
