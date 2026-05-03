# ESP32 多功能桌面小工具

基于 ESP32 + Arduino 框架的多功能桌面设备，集时钟、温湿度监测、闹钟、小游戏于一体。

本人第一个尝试开源的嵌入式仓库，欢迎 Star 和 Issue！

- 实时时钟
- 温湿度监测
- 闹钟功能
- 背光调节
- 扫雷
- 小恐龙跑酷
- 俄罗斯方块
- 计算器
- WiFi 管理：支持多个 WiFi 预设，一键切换
- 计划添加其他小游戏

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

### 2. 替换为你所想要的图片

- 使用 Image2Lcd 取模：
- C语言数组
- 水平扫描
- 16位真彩色
- 包含头数据
- 高位在前
- img1 为壁纸，img_1-6 为游戏图标

### 3. PCB 制板文件与电路图见 hardware 文件夹，可直接用嘉立创打样



---

## 许可证

本项目基于 MIT 许可证开源，详见 LICENSE 文件。

---

## 致谢

- [Adafruit](https://github.com/adafruit) 提供的传感器和屏幕驱动库
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) 提供开发框架
- 本项目的恐龙游戏部分，修改自 [pan1024](https://github.com/pan1024) 的开源项目 [google-dinosaur-game-for-esp32](https://github.com/pan1024/google-dinosaur-game-for-esp32)，已根据本项目需求进行调整，感谢原作者的工作。

---

版本 3.0.0
------2026.5.3 潘航宇


# ESP32 Multi-functional Desktop Gadget

A multi-functional desktop device based on ESP32 + Arduino framework, integrating clock, temperature & humidity monitoring, alarm, and mini-games.

My first open-source embedded repository. Stars and Issues are welcome!

- Real-time Clock
- Temperature & Humidity Monitoring
- Alarm Function
- Backlight Adjustment
- Minesweeper
- Dino Run
- Tetris
- Calculator
- WiFi Management: support multiple WiFi presets with one-key switching
- Planning to add more mini-games

## Hardware List

| Component | Quantity | Description |
|-----------|----------|-------------|
| ESP32-C3 Dev Board | 1 | Main Controller |
| ST7735 Display | 1 | 128x128 SPI Interface |
| DHT11 Sensor | 1 | Temperature & Humidity |

### Wiring Table

| ESP32 Pin | Peripheral |
|-----------|------------|
| GPIO 7 | Backlight BL |
| GPIO 5 | Up Button |
| GPIO 9 | Down Button |
| GPIO 10 | Left Button |
| GPIO 6 | OK Button |
| GPIO 21 | DHT11 Data |
| GPIO 8 | Buzzer |

Note: The Right button is triggered by pressing DOWN + LEFT simultaneously.

## Quick Start

### 1. Install Arduino IDE and ESP32 Support

- Download [Arduino IDE](https://www.arduino.cc/en/software)
- Go to `File` → `Preferences` → `Additional Boards Manager URLs` and add:
https://espressif.github.io/arduino-esp32/package_esp32_index.json

### 2. Replace with Your Own Images

- Use Image2Lcd for bitmap conversion:
- C array
- Horizontal scan
- 16-bit true color
- Include head data
- High byte first
- img1 is the wallpaper, img_1-6 are game icons.

### 3. PCB fabrication files and schematics are in the hardware folder, ready for direct use with JLCPCB.



---

## License

This project is open-sourced under the MIT License. See the LICENSE file for details.

---

## Acknowledgments

- [Adafruit](https://github.com/adafruit) for sensor and display driver libraries
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) for the development framework
- The Dino game in this project is modified from [pan1024](https://github.com/pan1024)'s open-source project [google-dinosaur-game-for-esp32](https://github.com/pan1024/google-dinosaur-game-for-esp32). It has been adapted to fit this project's needs. Thanks to the original author.

---

Version 3.0.0
------2026.5.3 Pan Hangyu