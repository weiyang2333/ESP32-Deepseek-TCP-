#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "MQTT.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_http_client.h"

// 写的到前面 文件测试可以使用 需要对应修改你的设备 ID等等 
// 可以上传物模型数据
// 物模型采用 one_json协议
// 下发数据采用HTTP协议 在http文件 与deepseek采用相同的协议模式 


#if A
bool mqtt_connect_flag = false; 
esp_mqtt_client_handle_t client_mqtt;//MQTT句柄
char OneNet_data_rev[1024]; //获取数据返回给全局变量给deepseek发送

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_M, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, ONENET_TOPIC_DP_SUBSCRIBE, 0);
            // ESP_LOGI(TAG_M, "sent subscribe successful, msg_id=%d", msg_id);
             if(msg_id != -1)
            {
                mqtt_connect_flag=true;
                mqtt_sta =MQTT_STATE_CONNECTED;
            }else
            {
                mqtt_connect_flag=false;
                mqtt_sta =MQTT_STATE_UNKNOWN;
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_M, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_M, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            vTaskDelay(100 / portTICK_PERIOD_MS); // 延迟让连接稳定
            msg_id = mqtt_data_publish("Thick_section",130);
            // ESP_LOGI(TAG_M, "sent publish successfully, msg_id=%d", msg_id);
            mqtt_connect_flag=true;
            mqtt_sta =MQTT_STATE_CONNECTED;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            mqtt_connect_flag=false;
            mqtt_sta =MQTT_STATE_UNKNOWN;
            ESP_LOGI(TAG_M, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_M, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_M, "MQTT_EVENT_DATA");

            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            if (strncmp(event->topic, ONENET_TOPIC_DP_SUBSCRIBE, event->topic_len) == 0) {
            // 平台下发指令处理
            cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);
            if (root) {
                cJSON *params = cJSON_GetObjectItem(root, "params");
                if (params) {
                    cJSON *num = cJSON_GetObjectItem(params, "number");
                    if (cJSON_IsNumber(num)) {
                        printf("收到平台下发数字 number: %d\n", num->valueint);
                        // 做相应处理
                    }
                }
                cJSON_Delete(root);
            }
        }
            memcpy(OneNet_data_rev, event->data, event->data_len);  //获取数据返回给全局变量给deepseek发送
            break;
        case MQTT_EVENT_ERROR:
            mqtt_sta = MQTT_STATE_ERROR;
            ESP_LOGI(TAG_M, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG_M, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG_M, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    mqtt_event_handler_cb(event_data);
}

/*
    param: theme   物模型名  
    value: 上传数据
*/

void mqtt_app_init(void)
{
    //结构体赋值  ESP-IDF v5.x
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = ESP_MQTT_URI,
    .broker.address.port = ESP_MQTT_PORT,
    .session = {
        .keepalive = 60  // 设置心跳间隔为 60 秒
    },

    .network = {
        .reconnect_timeout_ms = 5000  // 可选，断线后 5 秒重连
    },
    .credentials = {
        .client_id = ESP_CLIENT_ID,
        .username = ESP_PRODUCT_ID,
        .authentication.password = ESP_MQTT_TOKEN
    }};
    //初始化
     client_mqtt = esp_mqtt_client_init(&mqtt_cfg);
     //注册回调
    esp_mqtt_client_register_event(client_mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, client_mqtt);
    //启动
    esp_mqtt_client_start(client_mqtt);
}
int mqtt_data_publish(const char* theme, int32_t value)
{
    char data_str[256];
    int msg_id = -1;
    // 添加双引号包裹字符串类型的 value
    snprintf(data_str, sizeof(data_str),
         "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"%s\":{\"value\":\%ld\}}}",
         theme, value);
    if (mqtt_connect_flag)
    {
        msg_id = esp_mqtt_client_publish(client_mqtt, ONENET_TOPIC_DP_PUBLISH, data_str, 0, 0, 0);
        // msg_id = esp_mqtt_client_publish(client_mqtt);
        ESP_LOGI("DEBUG", "Sending JSON: %s", data_str);
        if (msg_id != -1){
            ESP_LOGI(TAG_M, "Sent publish successful, msg_id=%d", msg_id);
        }
        else{
            ESP_LOGE(TAG_M, "Failed to publish message");
        }
    }
    else
    {
        ESP_LOGW(TAG_M, "MQTT client is not connected");
    }
    return msg_id;
}

//以上代码是通过本设备向云平台发送数据 接下来采取A设备上传数据B设备接收数据
// 而我们是B设备则采取Https读取网站物模型数据 这样可以在离线的情况下去读最新数据
//本次任务仅读取数据因此不使用该模块



#endif
