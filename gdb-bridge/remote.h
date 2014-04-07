#ifndef REMOTE_H
#define REMOTE_H

#define SOCKET_BUFFER_SIZE 1024
struct FLContext;
int processMessage(const char *buf, int size, int conn, struct FLContext *handle);

#endif
