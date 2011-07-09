#ifndef REMOTE_H
#define REMOTE_H

#define SOCKET_BUFFER_SIZE 1024
struct I68K;
int processMessage(const char *buf, int size, int conn, struct I68K *cpu);

#endif
