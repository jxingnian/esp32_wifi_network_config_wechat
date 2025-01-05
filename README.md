# ESP32 WiFi配网项目

这是一个基于ESP32的WiFi配网项目，通过微信小程序或者网页实现WiFi配置和管理功能。

## 功能特性

- 📱 微信小程序配网界面
  - 扫描周围WiFi
  - 输入WiFi密码进行配网
  - 查看ESP32连接状态
  - 删除已保存的WiFi配置

- 🛜 ESP32功能
  - AP+STA双模式工作
  - 自动保存WiFi配置
  - 支持断电记忆
  - 自动重连机制
  - Web服务器接口

## 技术栈

- ESP32开发板
- ESP-IDF v5.0.2
- 微信小程序

## API接口

### 1. 获取ESP32状态
- URL: `http://192.168.4.1:8080/get_status`
- 方法: `GET`
- 响应示例:
```json
{
  "ssid": "WiFi名称",
  "connected": true
}
```

### 2. 配置WiFi
- URL: `http://192.168.4.1:8080/config`
- 方法: `POST`
- 请求体:
```json
{
  "ssid": "WiFi名称",
  "password": "WiFi密码"
}
```
- 响应示例:
```json
{
  "status": "success",
  "message": "WiFi配置已提交，正在连接..."
}
```

### 3. 删除WiFi配置
- URL: `http://192.168.4.1:8080/delete_wifi`
- 方法: `POST`
- 响应示例:
```json
{
  "status": "success",
  "message": "WiFi配置已删除"
}
```

## 使用说明

1. ESP32首次启动会创建一个AP热点
   - 默认SSID: `ESP32_XXXX`
   - 默认密码: 无

2. 使用微信小程序连接到ESP32
   - 打开小程序
   - 等待发现ESP32设备
   - 选择要连接的WiFi并输入密码
   - 等待配网完成

3. 配网成功后
   - ESP32会自动连接到配置的WiFi
   - 同时保持AP模式，方便后续管理
   - 断电后会自动重连已保存的WiFi

## 开发环境搭建

1. 安装ESP-IDF
```bash
# 下载ESP-IDF v5.0.2
git clone -b v5.0.2 --recursive https://github.com/espressif/esp-idf.git

# 运行安装脚本
cd esp-idf
./install.bat
```

2. 编译和烧录
```bash
# 设置环境变量
. ./export.sh

# 编译项目
idf.py build

# 烧录到ESP32
idf.py -p (串口号) flash
```

## 注意事项

1. 确保ESP-IDF版本为v5.0.2
2. 首次配网前需要将手机连接到ESP32的AP热点
3. 配网成功后，ESP32会同时工作在AP和STA模式
4. 如果配置的WiFi连接失败，会自动重试5次
5. EspWifiNetworkConfigwechat为小程序代码
5、项目已加密，建议在menuconfig先取消加密

## 贡献

欢迎提交Issue和Pull Request！

## 许可证

MIT License
