idf_component_register(SRCS "main.c" "wifi_manager.c" "http_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_http_server nvs_flash json spiffs)
