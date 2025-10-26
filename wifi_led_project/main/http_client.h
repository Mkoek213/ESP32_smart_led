#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <cstring>

/**
 * @brief HTTP Client Class
 * Performs HTTP GET requests to external servers
 */
class HTTPClient {
private:
    const char* server;
    int port;
    const char* path;
    int recv_buf_size;
    char* recv_buf;
    
public:
    HTTPClient(const char* server_name, int server_port, const char* request_path, int buffer_size);
    ~HTTPClient();
    
    bool perform_get_request();
};

#endif // HTTP_CLIENT_H

