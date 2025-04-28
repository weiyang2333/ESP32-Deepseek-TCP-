#include "esp_spiffs.h"
#include "tcp_server.h"

void txt_init(){
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,   // 允许同时打开的最大文件数
        .format_if_mount_failed = true  // 挂载失败时格式化
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "SPIFFS 挂载失败 (%s)", esp_err_to_name(ret));
        return;
    }
        ESP_LOGI("SPIFFS", "SPIFFS 挂载成功");
    char path[64];
    snprintf(path, sizeof(path), "/spiffs/%s", "Yarn_data.txt");
    FILE* file = fopen("/spiffs/Yarn_data.txt", "w");
    if(file == NULL)
    {
        ESP_LOGI("Main","文件%s不存在,即将创立文件",path);
        file = fopen(path, "w");
        if (file == NULL) {
            ESP_LOGE("main", "无法创建文件 %s", path);
        }
        else{ESP_LOGI("main", "文件 %s 创建成功", path);}
    }
    else {
        fclose(file);// 文件存在，关闭文件句柄
    }
}
void write_json_txt(cJSON *data){
    char path[64];
    snprintf(path, sizeof(path), "/spiffs/%s", "Yarn_data.txt");
    FILE *file = fopen(path, "a+");  // 以写入模式打开文件
    if (data != NULL) {
        char *json_string = cJSON_Print(data);  // 格式化 JSON
        if (json_string) {
            fprintf(file, "%s", json_string);  // 写入 JSON
            free(json_string);  // 释放 JSON 字符串
        } else {
            ESP_LOGE("main", "cJSON_Print 失败");
        }
    }
    fclose(file); // 关闭文件
    ESP_LOGI("main", "文件 %s 写入完成", path);
}
void write_txt(const char *data){
    char path[64];
    snprintf(path, sizeof(path), "/spiffs/%s", "Yarn_data.txt");
    FILE *file = fopen(path, "a+");  // 以写入模式打开文件
    fprintf(file, "%s", data);  // 写入 JSON
    fclose(file); // 关闭文件
    ESP_LOGI("main", "文件 %s 写入完成", path);
}
void read_txt_file() {
    char path[64];
    snprintf(path, sizeof(path), "/spiffs/%s", "Yarn_data.txt");
    FILE *file = fopen(path, "r");
    if (!file) {
        ESP_LOGE("main", "无法打开文件 %s", path);
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), file)) { // 按行读取
        ESP_LOGI("main", "文件:%s", line);
    }
    fclose(file);
    ESP_LOGI("main", "文件 %s 读取完成", path);
}