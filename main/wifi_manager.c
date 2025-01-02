#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_manager.h"

// WiFi配置参数
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID        // WiFi名称
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD    // WiFi密码
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL     // WiFi信道
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN     // 最大连接数

static const char *TAG = "wifi_manager";  // 日志标签

#define MAX_RETRY_COUNT 5
static int s_retry_num = 0;

// WiFi事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED:
                wifi_event_ap_staconnected_t* ap_event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "设备 "MACSTR" 已连接, AID=%d",
                         MAC2STR(ap_event->mac), ap_event->aid);
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                wifi_event_ap_stadisconnected_t* ap_disc_event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "设备 "MACSTR" 已断开连接, AID=%d",
                         MAC2STR(ap_disc_event->mac), ap_disc_event->aid);
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START，尝试连接到AP...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED，已连接到AP");
                s_retry_num = 0; // 重置重试计数
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
                ESP_LOGW(TAG, "WiFi断开连接，原因:%d", event->reason);
                if (s_retry_num < MAX_RETRY_COUNT) {
                    ESP_LOGI(TAG, "重试连接到AP... (%d/%d)", s_retry_num + 1, MAX_RETRY_COUNT);
                    esp_wifi_connect();
                    s_retry_num++;
                } else {
                    ESP_LOGW(TAG, "WiFi连接失败，达到最大重试次数");
                    // 保存当前状态到NVS
                    nvs_handle_t nvs_handle;
                    esp_err_t err = nvs_open("wifi_state", NVS_READWRITE, &nvs_handle);
                    if (err == ESP_OK) {
                        nvs_set_u8(nvs_handle, "connection_failed", 1);
                        nvs_commit(nvs_handle);
                        nvs_close(nvs_handle);
                    }
                }
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "获取到IP地址:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0; // 重置重试计数
            // 保存成功状态到NVS
            nvs_handle_t nvs_handle;
            esp_err_t err = nvs_open("wifi_state", NVS_READWRITE, &nvs_handle);
            if (err == ESP_OK) {
                nvs_set_u8(nvs_handle, "connection_failed", 0);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
            }
        }
    }
}

// 初始化WiFi软AP
esp_err_t wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());  // 初始化底层TCP/IP堆栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // 创建默认事件循环
    esp_netif_create_default_wifi_ap();  // 创建默认WIFI AP
    esp_netif_create_default_wifi_sta(); // 创建默认WIFI STA

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // 使用默认WiFi初始化配置
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));  // 初始化WiFi

    // 注册WiFi事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    // 配置AP参数
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = true
            },
        },
    };

    // 如果没有设置密码，使用开放认证
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // 设置WiFi为APSTA模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    // 设置AP配置
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // 尝试从NVS读取保存的WiFi配置
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        wifi_config_t sta_config;
        size_t size = sizeof(wifi_config_t);
        err = nvs_get_blob(nvs_handle, "sta_config", &sta_config, &size);
        if (err == ESP_OK) {
            // 检查是否之前连接失败
            uint8_t connection_failed = 0;
            nvs_get_u8(nvs_handle, "connection_failed", &connection_failed);
            
            if (!connection_failed) {
                ESP_LOGI(TAG, "找到已保存的WiFi配置，SSID: %s", sta_config.sta.ssid);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
            } else {
                ESP_LOGW(TAG, "上次WiFi连接失败，跳过自动连接");
            }
        }
        nvs_close(nvs_handle);
    }

    // 启动WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi初始化完成. SSID:%s 密码:%s 信道:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    return ESP_OK;
}

#define DEFAULT_SCAN_LIST_SIZE 10  // 默认扫描列表大小

// 扫描周围WiFi网络
esp_err_t wifi_scan_networks(wifi_ap_record_t **ap_records, uint16_t *ap_count)
{
    esp_err_t ret;
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;

    // 分配内存用于存储扫描结果
    *ap_records = malloc(DEFAULT_SCAN_LIST_SIZE * sizeof(wifi_ap_record_t));
    if (*ap_records == NULL) {
        ESP_LOGE(TAG, "为扫描结果分配内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 配置扫描参数
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 120,
        .scan_time.active.max = 150,
    };

    // 开始扫描
    ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "开始扫描失败");
        free(*ap_records);
        *ap_records = NULL;
        return ret;
    }

    // 获取扫描结果
    ret = esp_wifi_scan_get_ap_records(&number, *ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取扫描结果失败");
        free(*ap_records);
        *ap_records = NULL;
        return ret;
    }

    // 获取找到的AP数量
    ret = esp_wifi_scan_get_ap_num(ap_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取扫描到的AP数量失败");
        free(*ap_records);
        *ap_records = NULL;
        return ret;
    }

    // 限制AP数量不超过默认扫描列表大小
    if (*ap_count > DEFAULT_SCAN_LIST_SIZE) {
        *ap_count = DEFAULT_SCAN_LIST_SIZE;
    }

    // 打印扫描结果
    ESP_LOGI(TAG, "发现 %d 个接入点:", *ap_count);
    for (int i = 0; i < *ap_count; i++) {
        ESP_LOGI(TAG, "SSID: %s, 信号强度: %d", (*ap_records)[i].ssid, (*ap_records)[i].rssi);
    }

    return ESP_OK;
}
