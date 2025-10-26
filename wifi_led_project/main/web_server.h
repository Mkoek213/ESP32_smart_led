#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Simple Web Server Class
 * Provides a basic web interface for device configuration and status
 */
class WebServer {
private:
    int server_socket;
    bool running;
    TaskHandle_t server_task_handle;
    
    static void server_task_wrapper(void* arg);
    void server_task();
    
    void handle_client(int client_sock);
    void send_index_page(int sock);
    void send_status_page(int sock);
    void send_404(int sock);
    
public:
    WebServer();
    ~WebServer();
    
    bool start(int port);
    void stop();
};

#endif // WEB_SERVER_H

