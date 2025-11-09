#include "http_client.h"
#include "config.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

bool http_client_get(const char* server, int port, const char* path) {
    struct addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo* result = nullptr;
    if (getaddrinfo(server, port_str, &hints, &result) != 0 || result == nullptr) {
        return false;
    }

    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(result);
        return false;
    }

    if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(result);
        return false;
    }

    char request[128];
    int request_length = snprintf(request,
                                  sizeof(request),
                                  "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
                                  path,
                                  server);
    send(sock, request, request_length, 0);

    char buffer[HTTP_RECV_BUF_SIZE];
    bool got_response = false;
    int received;
    while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        got_response = true;
    }

    if (received < 0) {
        close(sock);
        freeaddrinfo(result);
        return false;
    }

    if (got_response) {
        printf("\n");
    }

    close(sock);
    freeaddrinfo(result);
    return got_response;
}

