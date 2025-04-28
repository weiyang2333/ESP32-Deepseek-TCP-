#ifndef FILE_H
#define FILE_H
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
void read_txt_file();
void write_txt(const char *data);
void write_json_txt(cJSON *data);
void txt_init();

#endif // TCP_SERVER_H