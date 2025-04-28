#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "simple_wifi_sta.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "file.h"
#include "tcp_server.h"
#define WIFI_CONNECT_BIT    BIT0
static EventGroupHandle_t s_wifi_ev = NULL;
static const char* TAG = "main";
//ds秘钥 此处为私钥 
#define DEEPSEEK_API_KEY "Bearer sk-3b261465426c4899bb50f3e00978cf89"  //这里替换为你的API
#define DEEPSEEK_API_URL "https://api.deepseek.com/v1/chat/completions"
#define MAX_OUTPUT_BUFFER_LEN   4096
static  char output_buffer[MAX_OUTPUT_BUFFER_LEN] = {0};   //用于接收通过http协议返回的数据
static int output_len = 0;
int response_num = 0;
int data_read=0;
#define CLIENT_HTTP_RECEIVE_BIT BIT0
static EventGroupHandle_t   s_client_http_event = NULL; //此事件用于通知http接收完成
static cJSON *messages = NULL; // 存储对话历史10条 
#define MAX_HISTORY 20 //消息最多保留20条
int Respose_Station = 1;//通信成功的状态位

//此处为请求结构体配置
#define DEFAULT_HTTP_CONFIG()  \
    ((esp_http_client_config_t){  \
        .url = DEEPSEEK_API_URL,       \
        .event_handler = http_client_event_handler, \
        .method = HTTP_METHOD_POST,    \
        .transport_type = HTTP_TRANSPORT_OVER_SSL, \
        .cert_pem = NULL,              \
        .timeout_ms = 200000           \
    })

static esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:    //错误事件
            //ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:    //连接成功事件
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:    //发送头事件
            //ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:    //接收头事件
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:    //接收数据事件
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            xEventGroupClearBits(s_client_http_event,CLIENT_HTTP_RECEIVE_BIT);
            if(output_len + evt->data_len < MAX_OUTPUT_BUFFER_LEN)
            {
                memcpy(&output_buffer[output_len], evt->data,evt->data_len);
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:    //会话完成事件
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            xEventGroupSetBits(s_client_http_event,CLIENT_HTTP_RECEIVE_BIT);
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:    //断开事件
            //ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            //ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

static esp_err_t parse_ds(char* ds_js)
{
    cJSON *wt_js = cJSON_Parse(ds_js);
    if(!wt_js)
    {
        ESP_LOGI(TAG,"invaild json format");
        Respose_Station = 0;
        return ESP_FAIL;
    }
    cJSON *choices = cJSON_GetObjectItem(wt_js,"choices");
    if (choices && cJSON_IsArray(choices)) {
        cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
        if (first_choice) {
            cJSON *message = cJSON_GetObjectItem(first_choice, "message");
            if (message) {
                cJSON *content = cJSON_GetObjectItem(message, "content");
                if (content && cJSON_IsString(content)) {
                    // ESP_LOGI("System", "DeepSeek 回复: %s", content->valuestring);
                    // write_txt(content->valuestring);
                    cJSON *message_ds = cJSON_CreateObject();
                    cJSON_AddStringToObject(message_ds, "role", "assistant");
                    cJSON_AddStringToObject(message_ds, "content", content->valuestring);
                    tcp_send_task(content->valuestring);
                    cJSON_AddItemToArray(messages, message_ds);
                    Respose_Station = 1;
                }
            }
        }
    }
    cJSON_Delete(wt_js);  // **释放 JSON 内存**
    return ESP_OK;  // **保证有返回值**

}
//修改用户发向Ds的内容
char *create_post_data(const char *user_input) {
    if (messages == NULL) {
        messages = cJSON_CreateArray(); // 初始化消息数组
        ESP_LOGE(TAG, "create_messages !!!");
    }

    cJSON *root = cJSON_CreateObject();
    // Possible values: [deepseek-chat, deepseek-reasoner] 二选一
    cJSON_AddStringToObject(root, "model", "deepseek-chat"); 
    //frequency_penalty Possible values[-2,2] 默认为0  为正降低模型重复相同内容的可能性。
    // cJSON_AddNumberToObject(root, "frequency_penalty",0.5); 

    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", user_input);

    // **检查历史记录是否超限**
    while (cJSON_GetArraySize(messages) > MAX_HISTORY) {
        cJSON_DeleteItemFromArray(messages, 0);
    }
    cJSON_AddItemToArray(messages, message);
    // 深拷贝 messages 避免messages后续被删除变为野指针而无法调用
    cJSON *messages_copy = cJSON_Duplicate(messages, 1); 
    cJSON_AddItemToObject(root, "messages", messages_copy);

    // **生成 JSON 字符串**
    char *post_data = cJSON_PrintUnformatted(root);
    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to generate JSON string!!!");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_Delete(root); // 释放 JSON 对象
    return post_data; // 记得 `free(post_data)` 释放内存
}



/** 发起http请求，获取DS数据
 * @param 无
 * @return ESP_OK or ESP_FAIL
*/
static esp_err_t ds_http_init(const char *user_input)
{
    esp_err_t ret_code = ESP_OK;
    //定义http配置结构体 调用开头define结构体 需要可在前面修改
    static esp_http_client_config_t http_config = DEFAULT_HTTP_CONFIG(); 
    //client作为客户端对象接受后续的post 等作用 初始化结构体
    esp_http_client_handle_t client = esp_http_client_init(&http_config);

    esp_http_client_set_header(client,"Authorization",DEEPSEEK_API_KEY);
    esp_http_client_set_header(client,"Content-Type","application/json");
    // 设置 POST 数据
    if(user_input == NULL)
    {
        printf("暂未接到数据");
        return ESP_FAIL;
    }
    ESP_LOGI("System","用户提问：%s",user_input);
    char *post_datas = create_post_data(user_input);
    esp_http_client_set_post_field(client, post_datas, strlen(post_datas));  //把post 与client组装

    // 执行 HTTP 请求
    esp_err_t err = esp_http_client_perform(client); //发送请求~~zo~~zo~~

    ESP_LOGI("System", "开始读取分块数据...");
    data_read = esp_http_client_read(client, output_buffer, sizeof(output_buffer) - 1);
    ESP_LOGI("System", "收到数据: %d,若为0则为分块数据", data_read);
    parse_ds(output_buffer); //解析数据
    // 释放资源
    // 如果没有返回数据则 post_data为空 此时free一个野指针会导致内存泄漏进而报错
    if (post_datas) {
        // ESP_LOGI("DEBUG", "post_data 地址: %p", (void *)post_datas);
        ESP_LOGE("all_postdata","%s",post_datas);
        free(post_datas);
    }
    esp_http_client_cleanup(client);
    return ret_code;
}

/** wifi事件通知
 * @param 无
 * @return 无
*/
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
    ev = xEventGroupWaitBits(s_wifi_ev,WIFI_CONNECT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    if(ev & WIFI_CONNECT_BIT)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    char recv_buf[128];
    while(1){
    if (xQueueReceive(messageQueue, recv_buf, portMAX_DELAY) == pdTRUE) {
            do{
                if(strcmp(recv_buf, "exit") == 0){break;}
                ds_http_init(recv_buf);
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
