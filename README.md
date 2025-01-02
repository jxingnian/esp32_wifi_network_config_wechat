# ESP32 WiFi配网项目

By.星年

## 项目介绍

这是一个基于ESP32-S3的WiFi配网项目，提供了一个简单易用的Web界面来配置ESP32的WiFi连接。用户可以通过手机或电脑连接到ESP32的AP热点，然后通过Web界面扫描并连接到周围的WiFi网络。

### 特性

- 简洁美观的Web配置界面
- WiFi扫描和信号强度显示
- 支持查看当前WiFi连接状态
- 支持管理已保存的WiFi配置
- 支持删除不需要的WiFi配置
- 自动重连机制
- 实时状态更新

### 硬件要求

- 芯片：ESP32-S3
- Flash：16MB
- 支持WiFi功能

## 使用说明

### 1. 首次使用

1. ESP32启动后会创建一个名为"myssid"的WiFi热点
2. 默认密码：mypassword
3. 连接到该热点后，使用浏览器访问 http://192.168.4.1
4. 在Web界面上可以看到可用的WiFi网络列表
5. 选择想要连接的WiFi，输入密码进行连接

### 2. 修改AP热点配置

在menuconfig中配置

Example Configuration -> wifi ssid

Example Configuration -> wifi password

### 3. 查看WiFi状态

- Web界面会实时显示当前WiFi连接状态
- 显示已连接WiFi的信号强度
- 显示BSSID等详细信息

### 4. 管理保存的WiFi

- 查看已保存的WiFi列表
- 可以快速连接已保存的WiFi
- 支持删除不需要的WiFi配置

## 项目结构

- `/main` - 主要源代码
  - `main.c` - 程序入口和WiFi初始化
  - `http_server.c` - Web服务器和API实现
- `/spiffs` - Web界面文件
  - `index.html` - 主页面
- `/partitions.csv` - 分区表配置

---

## 注意

1.项目已开启加密

2.flash8M

3.关闭nvs加密

## 自定义配置

### 1. 修改分区表

项目使用了自定义的分区表，可以在 `partitions.csv`中修改。

### 2. 移植到其他项目

1. 复制 `http_server.c`和 `index.html`到目标项目
2. 确保目标项目包含必要的组件配置
3. 在主程序中调用 `start_webserver()`函数

## 开发环境

- ESP-IDF 版本：v5.0
- 芯片：esp32-s3
- 编译器：GCC
- 开发工具：VS Code + ESP-IDF 插件

## 许可证

MIT License

## 链接

- GitHub仓库：https://github.com/jxingnian/esp32_wifi_network_config
