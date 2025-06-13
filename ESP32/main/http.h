#ifndef __HTTP_H
#define __HTTP_H

#include "esp_http_client.h"

#define ONENET_URL  "https://iot-api.heclouds.com/thingmodel/query-device-property"
#define ONENET_DEVICE_ID "2445420986"   // 替换成你的设备ID
#define ONENET_DEVICE_NAME "test_chat"   // 替换成你的设备名字
#define ONENET_AUTH_STRING "version=2018-10-31&res=products%2F25UkgjsmFS%2Fdevices%2Ftest_chat&et=1847882652&method=md5&sign=uflyiIOmOXOaFAF99usM5w%3D%3D" //token
#define ONENET_API_KEY "RVNWVE5tdEZrMkJzUjlSanRxeU8yanpsNGNScHdqM0M="  // Onetnet设备key_accee
#define ONENET_PRODUCT_ID "25UkgjsmFS"    

//ds秘钥 此处为私钥 
#define DEEPSEEK_API_KEY "YOUR API OF Deepseek"  //这里替换为你的API
#define DEEPSEEK_API_URL "https://api.deepseek.com/v1/chat/completions" //Deepseek
#define MAX_OUTPUT_BUFFER_LEN  4096

#define CLIENT_HTTP_RECEIVE_BIT BIT0
extern int Respose_Station;//通信成功的状态位
// extern char detection_data[128]; //保留的onenet 物模型实时数据 你可以在mian函数中使用

extern EventGroupHandle_t s_client_http_event; //此事件用于通知http接收完成
void onenet_http_get_task(void *pvParameters);
char *create_post_data(const char *user_input);
esp_err_t ds_http_init(const char *user_input,int32_t onenet);
esp_err_t parse_ds(char* ds_js);
void parse_json_response(const char *json_str);


#endif
