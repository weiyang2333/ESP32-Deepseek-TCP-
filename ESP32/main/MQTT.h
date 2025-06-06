#ifndef __MQTT_H
#define __MQTT_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#define A 1   // 执行A设备发送数据设置1 
//具体去看ONENET文档
#define ESP_MQTT_URI "mqtt://mqtts.heclouds.com"  //mqtt://mqtts.heclouds.com
#define ESP_MQTT_PORT 1883
 
#define ESP_CLIENT_ID  "test_chat"//设备id
#define ESP_PRODUCT_ID "25UkgjsmFS"//产品id
#define ESP_MQTT_TOKEN "version=2018-10-31&res=products%2F25UkgjsmFS%2Fdevices%2Ftest_chat&et=1847882652&method=md5&sign=uflyiIOmOXOaFAF99usM5w%3D%3D"  //TOKEN
 
#define ONENET_TOPIC_SUB "$sys/25UkgjsmFS/test_chat/thing/property/post/reply"  //订阅 进汇报能不能成功
#define ONENET_TOPIC_DP_PUBLISH "$sys/25UkgjsmFS/test_chat/thing/property/post"   //上报
#define ONENET_TOPIC_DP_SUBSCRIBE "$sys/25UkgjsmFS/test_chat/thing/property/set"   // 下发数据 获取

#define ONENET_TOPIC_DA_PUBLISH  "$sys/25UkgjsmFS/test_chat/thing/property/set_reply"  //发布该话题
#define ONENET_TOPIC_DA_SUBSCRIBE  "$sys/25UkgjsmFS/test_chat/thing/property/set"     //订阅该话题


extern char OneNet_data_rev[1024];
 
//MQTT状态
typedef enum {
    MQTT_STATE_ERROR = -1,
    MQTT_STATE_UNKNOWN = 0,
    MQTT_STATE_INIT,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_TIMEOUT,
} mqtt_client_state_t;
 
static const char *TAG_M = "MQTT";//LOG头
// extern bool mqtt_connect_flag;//连接标志位 是否连接成功
static mqtt_client_state_t mqtt_sta = MQTT_STATE_UNKNOWN;//MQTT状态
 
 
 
//MQTT初始化
void mqtt_app_init(void);
//MQTT发布消息
int mqtt_data_publish(const char* theme, int32_t value);
//MQTT状态获取
mqtt_client_state_t mqtt_get_sta();
#else


#endif
#endif