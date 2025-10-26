#include "web_server.h"
#include "config.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <cstring>

static const char* TAG = "WEB_SERVER";

WebServer::WebServer() : server_socket(-1), running(false), server_task_handle(nullptr) {}

WebServer::~WebServer() {
    stop();
}

void WebServer::server_task_wrapper(void* arg) {
    WebServer* server = static_cast<WebServer*>(arg);
    server->server_task();
}

void WebServer::server_task() {
    char recv_buf[1024];
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        
        ESP_LOGI(TAG, "Client connected: %s", inet_ntoa(client_addr.sin_addr));
        
        int len = recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (len > 0) {
            recv_buf[len] = '\0';
            ESP_LOGI(TAG, "Request: %s", recv_buf);
            
            if (strstr(recv_buf, "GET / ") || strstr(recv_buf, "GET /index")) {
                send_index_page(client_sock);
            } else if (strstr(recv_buf, "GET /status")) {
                send_status_page(client_sock);
            } else {
                send_404(client_sock);
            }
        }
        
        close(client_sock);
    }
}

void WebServer::send_index_page(int sock) {
    const char* header = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Connection: close\r\n"
                       "\r\n";
    send(sock, header, strlen(header), 0);
    
    const char* html = 
        "<!DOCTYPE html><html><head>"
        "<title>ESP32 Config</title>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>"
        "body{font-family:Arial;margin:20px;background:#f0f0f0}"
        ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}"
        "h1{color:#333;text-align:center}"
        ".info{background:#e8f4f8;padding:15px;border-left:4px solid #2196F3;margin:10px 0}"
        "button{background:#2196F3;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}"
        "button:hover{background:#1976D2}"
        ".status{color:#4CAF50;font-weight:bold}"
        "</style>"
        "</head><body>"
        "<div class='container'>"
        "<h1>ESP32 Configuration Portal</h1>"
        "<div class='info'>"
        "<h2>Welcome!</h2>"
        "<p>You are connected to ESP32 Access Point</p>"
        "<p class='status'>AP Mode Active</p>"
        "</div>"
        "<h3>Quick Actions:</h3>"
        "<button onclick=\"location.href='/status'\">View Status</button>"
        "<h3>Device Information:</h3>"
        "<ul>"
        "<li><strong>AP SSID:</strong> " AP_SSID "</li>"
        "<li><strong>AP IP:</strong> 192.168.4.1</li>"
        "<li><strong>Max Clients:</strong> 4</li>"
        "</ul>"
        "<p style='text-align:center;color:#666;font-size:12px'>ESP32 C++ - Access Point Mode</p>"
        "</div>"
        "</body></html>";
    
    send(sock, html, strlen(html), 0);
}

void WebServer::send_status_page(int sock) {
    const char* header = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Connection: close\r\n"
                       "\r\n";
    send(sock, header, strlen(header), 0);
    
    const char* html = 
        "<!DOCTYPE html><html><head>"
        "<title>ESP32 Status</title>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>"
        "body{font-family:Arial;margin:20px;background:#f0f0f0}"
        ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}"
        "h1{color:#333}"
        ".status-ok{color:#4CAF50}"
        ".status-error{color:#f44336}"
        "button{background:#2196F3;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer}"
        "</style>"
        "</head><body>"
        "<div class='container'>"
        "<h1>Device Status</h1>"
        "<h3>Access Point:</h3>"
        "<p class='status-ok'>Running</p>"
        "<ul>"
        "<li>SSID: " AP_SSID "</li>"
        "<li>Channel: 1</li>"
        "</ul>"
        "<h3>Station Mode:</h3>"
        "<p>SSID: " EXAMPLE_ESP_WIFI_SSID "</p>"
        "<button onclick=\"location.href='/'\">Back to Home</button>"
        "</div>"
        "</body></html>";
    
    send(sock, html, strlen(html), 0);
}

void WebServer::send_404(int sock) {
    const char* response = "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "<h1>404 Not Found</h1>";
    send(sock, response, strlen(response), 0);
}

bool WebServer::start(int port) {
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return false;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(server_socket);
        return false;
    }
    
    if (listen(server_socket, 5) < 0) {
        ESP_LOGE(TAG, "Failed to listen");
        close(server_socket);
        return false;
    }
    
    ESP_LOGI(TAG, "Web server started on port %d", port);
    
    running = true;
    xTaskCreate(server_task_wrapper, "web_server_task", 4096, this, 5, &server_task_handle);
    
    return true;
}

void WebServer::stop() {
    running = false;
    if (server_task_handle) {
        vTaskDelete(server_task_handle);
        server_task_handle = nullptr;
    }
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

