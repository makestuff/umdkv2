#ifndef ESCAPE_H
#define ESCAPE_H

#include <makestuff.h>
#include "sock.h"

void setConnection(SOCKET conn);
bool isInterrupted(void);
void setNonBlocking(SOCKET conn, bool nonBlocking);

#endif
