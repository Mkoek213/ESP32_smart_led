#include "http_client.h"
#include "config.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <cstdio>

static const char* TAG = "HTTP_CLIENT";

HTTPClient::HTTPClient(const char* server_name, int server_port, const char* request_path, int buffer_size)
    : server(server_name), port(server_port), path(request_path), recv_buf_size(buffer_size) {
    recv_buf = new char[recv_buf_size];
}

HTTPClient::~HTTPClient() {
    delete[] recv_buf;
}

bool HTTPClient::perform_get_request() {
    int sock = -1;
    struct sockaddr_in dest_addr;
    
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Starting HTTP GET request to %s:%d%s", server, port, path);
    ESP_LOGI(TAG, "============================================");
    
    struct hostent *server_info = gethostbyname(server);
    if (server_info == nullptr) {
        ESP_LOGE(TAG, "Failed to resolve hostname: %s", server);
        return false;
    }
    
    ESP_LOGI(TAG, "DNS lookup successful");
    ESP_LOGI(TAG, "Server IP: %s", inet_ntoa(*(struct in_addr*)server_info->h_addr));
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return false;
    }
    ESP_LOGI(TAG, "Socket created successfully");
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    std::memcpy(&dest_addr.sin_addr.s_addr, server_info->h_addr, server_info->h_length);
    
    ESP_LOGI(TAG, "Connecting to %s:%d...", server, port);
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket connect failed: errno %d", errno);
        close(sock);
        return false;
    }
    ESP_LOGI(TAG, "TCP connection established!");
    
    char http_request[256];
    snprintf(http_request, sizeof(http_request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: ESP32-CPP\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, server);
    
    ESP_LOGI(TAG, "Sending HTTP GET request...");
    ESP_LOGI(TAG, "Request:\n%s", http_request);
    
    int sent = send(sock, http_request, std::strlen(http_request), 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send request: errno %d", errno);
        close(sock);
        return false;
    }
    ESP_LOGI(TAG, "HTTP request sent (%d bytes)", sent);
    
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Receiving HTTP response:");
    ESP_LOGI(TAG, "============================================");
    
    int total_received = 0;
    int len;
    while ((len = recv(sock, recv_buf, recv_buf_size - 1, 0)) > 0) {
        recv_buf[len] = '\0';
        printf("%s", recv_buf);
        total_received += len;
    }
    
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "HTTP response received successfully!");
    ESP_LOGI(TAG, "Total bytes received: %d", total_received);
    ESP_LOGI(TAG, "============================================");
    
    close(sock);
    ESP_LOGI(TAG, "Socket closed");
    
    return true;
}

