#include "simple_wifi_sta.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/ip_addr.h" 
#include <arpa/inet.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

//需要把这两个修改成你家WIFI，测试是否连接成功
#define DEFAULT_WIFI_SSID           "20243"
#define DEFAULT_WIFI_PASSWORD       "37202546"

static const char *TAG = "wifi";

//事件通知回调函数
static wifi_event_cb    wifi_cb = NULL;

/** 事件回调函数
 * @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{   
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:      //WIFI以STA模式启动后触发此事件
            esp_wifi_connect();         //启动WIFI连接
            break;
        case WIFI_EVENT_STA_CONNECTED:  //WIFI连上路由器后，触发此事件
            ESP_LOGI(TAG, "connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:   //WIFI从路由器断开连接后触发此事件
            esp_wifi_connect();             //继续重连
            ESP_LOGI(TAG,"connect to the AP fail,retry now");
            break;
        default:
            break;
        }
    }
    if(event_base == IP_EVENT)                  //IP相关事件
    {
        switch(event_id)
        {
                case IP_EVENT_STA_GOT_IP: //只有获取到路由器分配的IP，才认为是连上了路由器
                if(wifi_cb)
                    wifi_cb(WIFI_CONNECTED);
            
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                break;
                
        }
    }
}


//WIFI STA初始化
esp_err_t wifi_sta_init(wifi_event_cb f)
{   
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_init();
    esp_event_loop_create_default();
    
    // 创建默认的 STA 接口
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();  //  用这个，不要用 esp_netif_new！
    
    // 停掉 DHCP 客户端
    esp_netif_dhcpc_stop(netif);
    
    // 设置静态 IP
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 172, 20, 10, 3);      // ESP32 的静态 IP
    IP4_ADDR(&ip_info.gw, 172, 20, 10, 1);        // 默认网关（你的热点 IP）
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 240); // 子网掩码
    
    esp_netif_set_ip_info(netif, &ip_info);

    // 设置 DNS 服务器  太抽象了 下次还是使用自动动态IP好了省的还要来回改
    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = inet_addr("8.8.8.8");  // 常用 DNS
    dns.ip.type = IPADDR_TYPE_V4;
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);

    // 3. 绑定接口
    ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());  // 绑定默认事件处理
    ESP_ERROR_CHECK(esp_netif_attach_wifi_station(netif));      // 将 netif 绑定到 WiFi STA

    // 4. 初始化 WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. 注册事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // 6. 设置 WiFi 参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_WIFI_SSID,
            .password = DEFAULT_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    wifi_cb = f;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;
}
