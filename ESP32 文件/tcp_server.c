#include "tcp_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "cJSON.h"
#include "esp_log.h"
// 定义全局变量
int server_socket = -1;
int client_socket;
static char buffer[2048];
char buffer_rec[2048];
QueueHandle_t socketQueue; //定义队列通信句柄 全局调用
QueueHandle_t messageQueue; //发送信息前调用 接收信息函数的信息 队列

void tcp_recv_task(void *pvParameters); //提前声明接收信息没有回应方便调用
void tcp_send_task(const char *message); //一样

void tcp_server_init(void *pvParameters) {
    struct sockaddr_in server_addr;
    
    // 创建 TCP 服务器套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        ESP_LOGI("TCP","Failed to create socket");
        return;
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP); // 使用静态 IP 地址

    // 绑定套接字到端口
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGI("TCP","Failed to bind socket");
        close(server_socket);
        return;
    }
    ESP_LOGI("TCP","服务器正在监听 IP: %s, 端口: %d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    // 开始监听
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        ESP_LOGI("TCP","Failed to listen");
        close(server_socket);
        return;
    }
    ESP_LOGI("TCP","Server listening on port %d...\n", SERVER_PORT);
    

    // 接受客户端连接并启动数据处理任务
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    // ESP_LOGI("TCP","Client connected");
    while(1){
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            ESP_LOGI("TCP","Failed to accept client connection");
            close(server_socket);
            return;
        }
        xQueueSend(socketQueue, &client_socket, portMAX_DELAY);  //接入到客户端 直接放入队列
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;
        xTaskCreate(tcp_recv_task, "tcp_receive_task", 4096, pclient, 5, NULL);
        // xTaskCreate(tcp_send_task, "tcp_send_task", 4096, NULL, 5, NULL);
    }
}

void tcp_recv_task(void *pvParameters) {
    int bytes_received;
    while(1) {
        if (xQueueReceive(socketQueue, &client_socket, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("TCP", "Start receiving from new client");
            tcp_send_task("嗨嗨嗨,小老弟来啦？");
            while(1){
                memset(buffer_rec, 0, sizeof(buffer_rec));
                bytes_received = recv(client_socket, buffer_rec, sizeof(buffer_rec), 0);
                if (bytes_received <= 0) {
                    ESP_LOGI("TCP", "Client disconnected or recv failed");
                    close(client_socket);
                    break;  // 回到队列等待下一个 client_socket
                }
                    // ESP_LOGI("TCP", "Received from client: %s", buffer_rec);
                xQueueSend(messageQueue, buffer_rec, portMAX_DELAY);  //接入到客户端 直接放入队列
            }
        }
    }

    close(client_socket);  // 清理 socket
    vTaskDelete(NULL);     // 正确退出任务
}

void tcp_send_task(const char * const string) {
    // 从队列中获取客户端套接字
     //作为结尾字符串避免TCP长信息分块无法读取
    snprintf(buffer, sizeof(buffer), "%sEND", string); 
    vTaskDelay(pdMS_TO_TICKS(500));

    if (client_socket > 0) {
        send(client_socket, buffer, strlen(buffer), 0); //标志位一般为0
        ESP_LOGI("TCP", "Sent message to client: %s", string);
    } else {
        ESP_LOGI("TCP", "No client connected yet");
    }
}
