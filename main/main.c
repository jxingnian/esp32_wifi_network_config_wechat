/*
 * @Author: jxingnian j_xingnian@163.com
 * @Date: 2025-01-01 11:27:58
 * @LastEditors: xingnina j_xingnian@163.com
 * @LastEditTime: 2025-01-02 21:55:40
 * @FilePath: \EspWifiNetworkConfig\main\main.c
 * @Description: WiFi配网主程序
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "mqtt_client.h"
#include "mqtt_config.h"

static const char *TAG = "main";

// WiFi连接成功后的回调函数
static void wifi_connected_handler(void)
{
    ESP_LOGI(TAG, "WiFi已连接，正在初始化MQTT客户端...");
    
    // 配置MQTT客户端
    mqtt_client_config_t mqtt_cfg = {
        .broker_url = CONFIG_MQTT_BROKER_URL,
        .port = CONFIG_MQTT_BROKER_PORT,
        .client_id = CONFIG_MQTT_CLIENT_ID,
        .username = NULL,  // 如果需要认证，在这里设置
        .password = NULL   // 如果需要认证，在这里设置
    };
    
    // 初始化MQTT客户端
    esp_err_t err = mqtt_client_init(&mqtt_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return;
    }
    
    // 启动MQTT客户端
    err = mqtt_client_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT客户端启动失败");
        return;
    }
    
    ESP_LOGI(TAG, "MQTT客户端启动成功");
}

// 初始化SPIFFS
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,   // 最大打开文件数
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

void app_main(void)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化SPIFFS
    ESP_ERROR_CHECK(init_spiffs());


    // 启动HTTP服务器
    ESP_ERROR_CHECK(start_webserver());
    ESP_LOGI(TAG, "系统初始化成功");
}