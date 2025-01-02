#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "esp_err.h"
#include "mqtt_client.h"

// MQTT客户端状态
typedef enum {
    MQTT_CLIENT_STATE_INIT = 0,
    MQTT_CLIENT_STATE_CONNECTED,
    MQTT_CLIENT_STATE_DISCONNECTED,
    MQTT_CLIENT_STATE_ERROR
} mqtt_client_state_t;

// MQTT客户端配置结构体
typedef struct {
    const char *broker_url;
    const char *client_id;
    const char *username;
    const char *password;
    int port;
} mqtt_client_config_t;

// 初始化MQTT客户端
esp_err_t mqtt_client_init(const mqtt_client_config_t *config);

// 启动MQTT客户端
esp_err_t mqtt_client_start(void);

// 停止MQTT客户端
esp_err_t mqtt_client_stop(void);

// 发布消息
esp_err_t mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);

// 订阅主题
esp_err_t mqtt_client_subscribe(const char *topic, int qos);

// 取消订阅主题
esp_err_t mqtt_client_unsubscribe(const char *topic);

// 获取MQTT客户端状态
mqtt_client_state_t mqtt_client_get_state(void);

#endif /* _MQTT_CLIENT_H_ */
