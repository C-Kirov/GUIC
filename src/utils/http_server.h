#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <windows.h>

HANDLE StartHttpServer(unsigned short port);
void StopHttpServer();
bool IsHttpServerRunning();

#endif
