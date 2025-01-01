/*
 * @Author: jxingnian j_xingnian@163.com
 * @Date: 2025-01-02 00:07:02
 * @Description: HTTP服务器实现
 */

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "http_server.h"
#include <sys/stat.h> 
static const char *TAG = "http_server";
static httpd_handle_t server = NULL;

// 处理根路径请求 - 返回index.html
static esp_err_t root_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;
    
    // 构建完整的文件路径
    strlcpy(filepath, "/spiffs/index.html", sizeof(filepath));
    
    // 获取文件信息
    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read file");
        return ESP_FAIL;
    }
    
    fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read file");
        return ESP_FAIL;
    }
    
    // 设置Content-Type
    httpd_resp_set_type(req, "text/html");
    
    // 发送文件内容
    char *chunk = malloc(CHUNK_SIZE);
    if (chunk == NULL) {
        fclose(fd);
        ESP_LOGE(TAG, "Failed to allocate memory for chunk");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory");
        return ESP_FAIL;
    }
    
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, CHUNK_SIZE, fd);
        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                free(chunk);
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);
    
    free(chunk);
    fclose(fd);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// 处理WiFi扫描请求
static esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    
    // 如果当前不是APSTA模式，切换到APSTA模式
    if (mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }

    // 清除之前的扫描结果
    ESP_LOGI(TAG, "清除之前的扫描结果");
    ESP_ERROR_CHECK(esp_wifi_clear_ap_list());
    
    // 配置扫描参数
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 0,
                .max = 100
            }
        }
    };
    
    ESP_LOGI(TAG, "开始WiFi扫描...");
    // 开始扫描
    esp_err_t scan_ret = esp_wifi_scan_start(&scan_config, true);
    if (scan_ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi扫描失败，错误码: %d", scan_ret);
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "{\"status\":\"error\",\"message\":\"Scan failed: %s\"}", esp_err_to_name(scan_ret));
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, err_msg, strlen(err_msg));
        return ESP_OK;
    }
    
    // 获取扫描结果
    uint16_t ap_count = 0;
    esp_err_t ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取AP数量失败，错误码: %d", ret);
        const char *response = "{\"status\":\"error\",\"message\":\"Failed to get AP count\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "找到 %d 个WiFi网络", ap_count);
    
    if (ap_count == 0) {
        const char *response = "{\"status\":\"success\",\"message\":\"No WiFi networks found\",\"networks\":[]}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }
    
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_records == NULL) {
        ESP_LOGE(TAG, "内存分配失败");
        const char *response = "{\"status\":\"error\",\"message\":\"Memory allocation failed\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }
    
    ret = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取AP记录失败，错误码: %d", ret);
        free(ap_records);
        const char *response = "{\"status\":\"error\",\"message\":\"Failed to get AP records\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }
    
    // 创建JSON响应
    cJSON *root = cJSON_CreateObject();
    cJSON *networks = cJSON_CreateArray();
    
    for (int i = 0; i < ap_count; i++) {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", (char *)ap_records[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(ap, "authmode", ap_records[i].authmode);
        cJSON_AddItemToArray(networks, ap);
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_records[i].ssid, ap_records[i].rssi);
    }
    
    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddItemToObject(root, "networks", networks);
    
    char *json_response = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_response);
    
    free(json_response);
    cJSON_Delete(root);
    free(ap_records);
    
    return ESP_OK;
}

// 处理配网请求
static esp_err_t configure_post_handler(httpd_req_t *req)
{
    char buf[200];
    int ret, remaining = req->content_len;
    
    if (remaining > sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }
    
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    
    buf[ret] = '\0';
    
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *password = cJSON_GetObjectItem(root, "password");
    
    if (!ssid || !cJSON_IsString(ssid)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing SSID");
        return ESP_FAIL;
    }
    
    // 配置WiFi连接
    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, ssid->valuestring, sizeof(wifi_config.sta.ssid));
    if (password && cJSON_IsString(password)) {
        strlcpy((char *)wifi_config.sta.password, password->valuestring, sizeof(wifi_config.sta.password));
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    cJSON_Delete(root);
    
    const char *response = "{\"status\":\"success\",\"message\":\"WiFi配置已提交，正在连接...\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// URI处理结构
static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t scan = {
    .uri       = "/scan",
    .method    = HTTP_GET,
    .handler   = scan_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t configure = {
    .uri       = "/configure",
    .method    = HTTP_POST,
    .handler   = configure_post_handler,
    .user_ctx  = NULL
};

// 启动Web服务器
esp_err_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 8;
    
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &scan);
        httpd_register_uri_handler(server, &configure);
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Error starting server!");
    return ESP_FAIL;
}

// 停止Web服务器
esp_err_t stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    return ESP_OK;
}