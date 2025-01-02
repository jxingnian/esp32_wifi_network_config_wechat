#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "mqtt_client.h"
#include "mqtt_config.h"

static const char *TAG = "MQTT_CLIENT";

// MQTT客户端句柄
static esp_mqtt_client_handle_t mqtt_client = NULL;
static mqtt_client_state_t client_state = MQTT_CLIENT_STATE_INIT;

// MQTT事件处理函数
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected to broker");
            client_state = MQTT_CLIENT_STATE_CONNECTED;
            // 连接成功后订阅相关主题
            esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_CONTROL, MQTT_QOS_1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected from broker");
            client_state = MQTT_CLIENT_STATE_DISCONNECTED;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT Subscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT Unsubscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT Data received:");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error occurred");
            client_state = MQTT_CLIENT_STATE_ERROR;
            break;
            
        default:
            ESP_LOGI(TAG, "Other MQTT event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_client_init(const mqtt_client_config_t *config)
{
    if (mqtt_client != NULL) {
        ESP_LOGI(TAG, "MQTT client already initialized");
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config->broker_url,
        .broker.address.port = config->port,
        .credentials.client_id = config->client_id,
        .credentials.username = config->username,
        .credentials.authentication.password = config->password,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // 注册MQTT事件处理函数
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    return ESP_OK;
}

esp_err_t mqtt_client_start(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_mqtt_client_start(mqtt_client);
}

esp_err_t mqtt_client_stop(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_mqtt_client_stop(mqtt_client);
}

esp_err_t mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    if (mqtt_client == NULL || client_state != MQTT_CLIENT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "MQTT client not ready");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t mqtt_client_subscribe(const char *topic, int qos)
{
    if (mqtt_client == NULL || client_state != MQTT_CLIENT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "MQTT client not ready");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t mqtt_client_unsubscribe(const char *topic)
{
    if (mqtt_client == NULL || client_state != MQTT_CLIENT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "MQTT client not ready");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

mqtt_client_state_t mqtt_client_get_state(void)
{
    return client_state;
}
