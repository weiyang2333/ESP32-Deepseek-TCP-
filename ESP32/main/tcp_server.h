#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
// 服务器端口号
#define SERVER_PORT 8083
// 最大连接数
#define MAX_CONNECTIONS 2  //最大连接数2
// 服务器静态 IP 地址（可以根据需要修改）
#define IP "172.20.10.3"
// 声明全局变量
extern int server_socket;
extern int client_socket;
extern char buffer_rec[2048];
extern QueueHandle_t socketQueue;   //传递队列消息 client_socket 句柄
extern QueueHandle_t messageQueue;   //传递队列消息 client_socket 句柄

void tcp_server_init(void *pvParameters); // 用于初始化连接
void tcp_recv_task(void *pvParameters); // 用于发送和接收数据
void tcp_send_task(const char *message); //发送信息

#endif // TCP_SERVER_H
