#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>

bool http_client_get(const char* server, int port, const char* path);

#endif

