# ESP32 多功能桌面小工具

 基于 ESP32 + Arduino 框架的多功能桌面设备，集时钟、温湿度监测、闹钟、小游戏于一体。

 本人第一个尝试开源的嵌入式仓库，欢迎 Star 和 Issue！

实时时钟
温湿度监测
闹钟功能
背光调节
小游戏
计算器
WiFi 管理：支持多个 WiFi 预设，一键切换

## 硬件清单

| 元件 | 数量 | 说明 |
|------|------|------|
| ESP32-C3 开发板 | 1 | 主控 |
| ST7735 显示屏 | 1 | 128x128 SPI 接口 |
| DHT11 传感器 | 1 | 温湿度采集 |

### 接线表

| ESP32 引脚 | 外设 |
|-----------|------|
| GPIO 7 | 背光 BL |
| GPIO 5 | 上键 UP |
| GPIO 9 | 下键 DOWN |
| GPIO 10 | 左键 LEFT |
| GPIO 6 | OK 键 |
| GPIO 21 | DHT11 数据 |
| GPIO 8 | 蜂鸣器 |

注意：右键通过同时按下 DOWN + LEFT 触发

## 快速开始

### 1. 安装 Arduino IDE 和 ESP32 支持

- 下载 [Arduino IDE](https://www.arduino.cc/en/software)
- 在 `文件` → `首选项` → `附加开发板管理器网址` 添加：
https://espressif.github.io/arduino-esp32/package_esp32_index.json
=======================================================================
许可证

本项目基于 MIT 许可证开源，详见 LICENSE 文件。
=======================================================================

致谢

- [Adafruit](https://github.com/adafruit) 提供的传感器和屏幕驱动库
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) 提供开发框架
- 本项目的恐龙游戏部分，修改自 [pan1024](https://github.com/pan1024) 的开源项目 [google-dinosaur-game-for-esp32]
(https://github.com/pan1024/google-dinosaur-game-for-esp32)，已根据本项目需求进行调整，感谢原作者的工作。