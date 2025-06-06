#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "simple_wifi_sta.h"
#include "cJSON.h"
#include "file.h"
#include "tcp_server.h"
#include "MQTT.h"
#include "http.h"

static const char* TAG = "main";
int statue = 0; //onnet状态码 为0不连接oennet 

static EventGroupHandle_t s_wifi_ev = NULL;
#define WIFI_CONNECT_BIT    BIT0

const char* first_message = " \
    如果纱线生产过程中，\
    会出现五种瑕疵，粗节的来源是前罗拉故障，细节的来源是饰纱切割机构故障，长缺节的来源是环锭加捻故障，短缺节的来源是饰纱喂入机构故障，\
    饰纱交错的来源是芯纱加捻以及罗拉装置故障，粗节、细节、长缺节、短缺节、饰纱交错的数量分别超过151，96，69，63，83，认为机构出现故障。\
    稍后我会和你传入具体的数据你看看数据判断设备故障程度 没有数据传入的视为正常";
    // 实时数据：粗节=%d，细节=%d，长缺节=%d，短缺节=%d，饰纱交错=%d,151,96,69,63,83;";
//标志位函数 wifi信号通知
void wifi_event_handler(WIFI_EV_e ev)
{
    if(ev == WIFI_CONNECTED)
    {
        xEventGroupSetBits(s_wifi_ev,WIFI_CONNECT_BIT);
    }
}

void app_main(void)
{
    socketQueue = xQueueCreate(5, sizeof(int)); //先创建队列方便后续发消息不创建会报错
    messageQueue = xQueueCreate(10, sizeof(char[128])); //后续信息跨文件调用
    // flash 初始化
    esp_err_t ret = nvs_flash_init();
    s_wifi_ev = xEventGroupCreate();
    EventBits_t ev = 0;
    s_client_http_event = xEventGroupCreate();
    //初始化WIFI，传入回调函数，用于通知连接成功事件
    wifi_sta_init(wifi_event_handler);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    xTaskCreate(tcp_server_init, "TCP Server Init", 2048, NULL, 5, NULL);
    xTaskCreate(onenet_http_get_task, "onenet_http_get_task", 6400, NULL, 5, NULL);
    // mqtt_app_init();  如果你需要上传什么数据的话可以在这里上传
    ev = xEventGroupWaitBits(s_wifi_ev,WIFI_CONNECT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    if(ev & WIFI_CONNECT_BIT)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    ds_http_init(first_message,0);  //特殊仅发送前置条件
    char recv_buf[1024];
    while(1){
    if (xQueueReceive(messageQueue, recv_buf, portMAX_DELAY) == pdTRUE) {
            if(strstr(recv_buf,"设备") || strstr(recv_buf ,"平台")) //设立此判断 只要语句中含有数据就使用瑕点情况汇报给DEEPseek
            {
                statue = 1;
            } 
            do{
                if(strcmp(recv_buf, "exit") == 0){break;}
                ds_http_init(recv_buf,statue);
            }while(!Respose_Station);
        }
    }
    ESP_LOGI(TAG,"提问进程结束");
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    return;
}