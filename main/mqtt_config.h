#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_

// MQTT服务器配置
#define CONFIG_MQTT_BROKER_URL "mqtt://mqtt.eclipseprojects.io"
#define CONFIG_MQTT_BROKER_PORT 1883
#define CONFIG_MQTT_CLIENT_ID "esp32_client"

// MQTT主题配置
#define MQTT_TOPIC_BASE          "esp32/device/"
#define MQTT_TOPIC_STATUS        MQTT_TOPIC_BASE "status"
#define MQTT_TOPIC_CONTROL       MQTT_TOPIC_BASE "control"
#define MQTT_TOPIC_DATA          MQTT_TOPIC_BASE "data"

// MQTT QoS配置
#define MQTT_QOS_0              0
#define MQTT_QOS_1              1
#define MQTT_QOS_2              2

// MQTT重连配置
#define MQTT_RECONNECT_TIMEOUT_MS    (10 * 1000)
#define MQTT_RECONNECT_MAX_ATTEMPTS  5

#endif /* _MQTT_CONFIG_H_ */
