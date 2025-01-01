/*
 * @Author: jxingnian j_xingnian@163.com
 * @Date: 2025-01-02 00:07:02
 * @Description: HTTP服务器头文件
 */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "esp_err.h"

#define FILE_PATH_MAX (128 + 128)
#define CHUNK_SIZE    (4096)

// 启动Web服务器
esp_err_t start_webserver(void);

// 停止Web服务器
esp_err_t stop_webserver(void);

#endif /* _HTTP_SERVER_H_ */