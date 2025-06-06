#include "esp_http_client.h"
#include "http.h"
#include "cJSON.h"
#include "tcp_server.h"
#include "esp_log.h"

#define DEFAULT_HTTP_CONFIG()  \
    ((esp_http_client_config_t){  \
        .url = DEEPSEEK_API_URL,       \
        .event_handler = http_client_event_handler, \
        .method = HTTP_METHOD_POST,    \
        .transport_type = HTTP_TRANSPORT_OVER_SSL, \
        .cert_pem = NULL,              \
        .timeout_ms = 200000           \
    });

EventGroupHandle_t s_client_http_event = NULL;
char detection_data[256] = {0};  // 创建用于保存onenet 返回数据
static int Detec_data = 0;  //设立他的目的是为了字符串拼接 如果有更好的方案可以替换

#define MAX_OUTPUT_BUFFER_LEN   4096
static  char output_buffer[MAX_OUTPUT_BUFFER_LEN] = {0};   //用于接收通过http协议返回的数据
static int output_len = 0;
int Respose_Station = 1; 
#define MAX_HISTORY 20 //消息最多保留20条
static cJSON *messages = NULL; // 存储对话历史10条 

static const char* HTTP_TAG = "HTTP_TAP";

esp_err_t http_client_event_handler(esp_http_client_event_t *evt);

void parse_json_response(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    cJSON *data_array = cJSON_GetObjectItem(root, "data");
    int array_size = cJSON_GetArraySize(data_array);
    Detec_data = 0;  // 起始拼接位置清零
    //因为网站返回的信息是分条的因此采用for循环读取全部组信息
    for (int i = 0; i < array_size; i++) {
        cJSON *item = cJSON_GetArrayItem(data_array, i);
        if (item) {
            cJSON *name = cJSON_GetObjectItem(item, "name");
            cJSON *value = cJSON_GetObjectItem(item, "value");
            if (cJSON_IsString(name) && cJSON_IsString(value)) {
                // printf("名称: %s, 数值: %s\n", name->valuestring, value->valuestring);
                int n = snprintf(detection_data + Detec_data,sizeof(detection_data) - Detec_data,
                                 "名称: %s, 数值: %s ",  // \n 加一个换行更整齐
                                 name->valuestring,
                                 value->valuestring);
                if (n > 0 && Detec_data + n < sizeof(detection_data)) {Detec_data += n;} 
                else { break;}  // 防止溢出
            }
        }
    }
    cJSON_Delete(root);
}

esp_err_t parse_ds(char* ds_js)
{
    cJSON *wt_js = cJSON_Parse(ds_js);
    if(!wt_js)
    {
        ESP_LOGI(HTTP_TAG,"invaild json format");
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

void onenet_http_get_task(void *pvParameters) {
    char onenet_url[256];
     
    snprintf(onenet_url, sizeof(onenet_url),  //注意这里我使用的是onejson作为数据协议，如果你使用数据流请进行更换URL
        ONENET_URL"?product_id=%s&device_name=%s",
        ONENET_PRODUCT_ID,
        ONENET_DEVICE_NAME
    );

    ESP_LOGI(HTTP_TAG,"url:%s",onenet_url);
    esp_http_client_config_t config = {
        .url = onenet_url,
         .method = HTTP_METHOD_GET,
        .event_handler = http_client_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "api-key", ONENET_API_KEY);
    esp_http_client_set_header(client, "Authorization", ONENET_AUTH_STRING);
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "Content-Type", "application/json");

    //清空之前缓存
    memset(detection_data, 0, sizeof(detection_data));
    Detec_data = 0;

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_read(client, output_buffer, sizeof(output_buffer) - 1);
    parse_json_response(output_buffer);
    ESP_LOGI(HTTP_TAG,"Look At:%s",detection_data);
    // if (err == ESP_OK) {
    //     ESP_LOGI(Main_TAG, "HTTPS Status = %d, Content_length = %lld",
    //              esp_http_client_get_status_code(client),
    //              esp_http_client_get_content_length(client));
    // } else {
    //     ESP_LOGE(Main_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    // }

    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

char *create_post_data(const char *user_input) {
    if (messages == NULL) {
        messages = cJSON_CreateArray(); // 初始化消息数组
        ESP_LOGE(HTTP_TAG, "create_messages !!!");
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
        ESP_LOGE(HTTP_TAG, "Failed to generate JSON string!!!");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_Delete(root); // 释放 JSON 对象
    return post_data; // 记得 `free(post_data)` 释放内存
}

esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:    //错误事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:    //连接成功事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:    //发送头事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:    //接收头事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:    //接收数据事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            xEventGroupClearBits(s_client_http_event,CLIENT_HTTP_RECEIVE_BIT);
            if(output_len + evt->data_len < MAX_OUTPUT_BUFFER_LEN)
            {
                memcpy(&output_buffer[output_len], evt->data,evt->data_len);
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:    //会话完成事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_ON_FINISH");
            xEventGroupSetBits(s_client_http_event,CLIENT_HTTP_RECEIVE_BIT);
            output_len = 0;
            // //对于onenet传回的数据是分批次的因此
            // detection_data[0] = '\0';   //事件结束我们清空字符串
            break;
        case HTTP_EVENT_DISCONNECTED:    //断开事件
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_DISCONNECTED");
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            //ESP_LOGI(Main_TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t ds_http_init(const char *user_input,int32_t onenet)
{
    int data_read=0; //获取数据状态码
    char result_datection_data[1024]; //拼接字符串 以及 接收到的Onenet json数据信息
    esp_err_t ret_code = ESP_OK;
    ESP_LOGI("System_O", "Onenet:收到数据: %ld",onenet);
    char *post_datas = NULL;  // 提前定义并初始化

    //定义http配置结构体 调用开头define结构体 需要可在前面修改
    esp_http_client_config_t deepseek_config = DEFAULT_HTTP_CONFIG();
    esp_http_client_handle_t client = esp_http_client_init(&deepseek_config);
    //设置头文件 
    esp_http_client_set_header(client,"Authorization",DEEPSEEK_API_KEY);
    esp_http_client_set_header(client,"Content-Type","application/json");
    snprintf(result_datection_data, sizeof(result_datection_data), "在当前瑕疵数量种类如下：%s", detection_data);
    // ESP_LOGI("System_O", "result_datection_data:收到数据: %s",result_datection_data);
    if(onenet == 1){user_input = result_datection_data;}
    post_datas = create_post_data(user_input);
    esp_http_client_set_post_field(client, post_datas, strlen(post_datas));  //把post 与client组装
    ESP_LOGI("System_O", "发送数据: %s",post_datas);
    esp_http_client_perform(client); //发送 zo~zo~zo~
    char* first_message = "粗节的来源是前罗拉故障，细节的来源是饰纱切割机构故障";
    if(strstr(user_input,first_message)){ESP_LOGI("System", "收到数据: %ld,返回数据", onenet);return 1;}  //如果检测到特定句子直接结束

    ESP_LOGI(HTTP_TAG, "开始读取分块数据...");
    data_read = esp_http_client_read(client, output_buffer, sizeof(output_buffer) - 1);
    ESP_LOGI(HTTP_TAG, "收到数据: %s", output_buffer);

    // if(strstr(user_input,first_message) || onenet == 1){ESP_LOGI("System", "收到数据: %ld,返回数据", onenet);return 1;}  //如果检测到特定句子直
    parse_ds(output_buffer); //解析数据
    // 释放资源
    // 如果没有返回数据则 post_data为空 此时free一个野指针会导致内存泄漏进而报错
    if(post_datas) {
        ESP_LOGE("all_postdata","%s",post_datas);
        free(post_datas);
    }
    esp_http_client_cleanup(client);
    return ret_code;
}