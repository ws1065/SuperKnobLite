# 第三方许可与来源说明

本仓库采用 MIT 许可（见 `LICENSE`）。仓库中包含或引用了以下第三方组件，
它们各自保留原有的版权与许可条款。

## 固件库依赖（PlatformIO 自动拉取，位于 `.pio/libdeps/`）

| 组件 | 作者 | 许可 |
|---|---|---|
| [Simple FOC](https://github.com/simplefoc/Arduino-FOC-Library) 2.2.1 | Antun Skuric | MIT |
| [lvgl](https://github.com/lvgl/lvgl) 8.2 | LVGL 项目 | MIT |
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) | Bodmer | BSD-3-Clause（源自 Adafruit_ILI9341） |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) 6.19 | Benoit Blanchon | MIT |
| [ESP32 BLE Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) | T-vK | MIT |
| [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) | Adafruit | MIT |
| [PubSubClient](https://github.com/knolleary/PubSubClient) | Nicholas O'Leary | MIT |
| ESP-IDF / Arduino-ESP32 框架 | Espressif | Apache-2.0 |

> `.pio/libdeps/` 已在 `.gitignore` 中排除——这些库由 `platformio.ini` 的
> `lib_deps` 声明自动拉取，不随仓库分发。使用 `pio run` 即可自动获取。

## 界面代码

`src/user_app/gui_super_knob.h` 中部分界面代码：

```
Copyright 2022 NXP
SPDX-License-Identifier: MIT
```

## 原始固件作者

本项目基于原始 SuperKnobLite 固件扩展而来，源码头部保留了原作者署名：

- `@Author: congsir`
- `@LastEditors: wenzheng 565402462@qq.com`

## 工具链

| 组件 | 许可 |
|---|---|
| [PlatformIO](https://platformio.org/) Core | Apache-2.0 |
| [esptool](https://github.com/espressif/esptool) | GPL-2.0（作为独立命令行工具调用，不随本项目分发） |

## 硬件设计文件

`SuperKnobLite 二代旋钮电路图及pcb/` 目录下的 Altium PcbDoc/PcbLib 属于二进制设计
文件，已在 `.gitignore` 中排除，不随仓库分发。

## 本项目新增代码

以下部分为本次扩展工作原创，采用 MIT 许可：

- `superknob_control.html` —— 串口控制台（Web Serial API）
- `tools/paced_esptool.py` —— 分片节流烧录包装
- `tools/knob_server.mjs` —— 电脑资源采集服务
- 固件中的串口协议扩展、触觉模式引擎、转速伺服、位置感知、屏幕诊断
- `README_cn.md`、`docs/技术方案.md`

---

**说明**：MIT 许可本身不要求对第三方组件单独授权或修改其许可；本文件仅为
透明度说明，便于使用者了解各组件来源。若需要分发二进制固件，请确保
`pio run` 拉取的第三方库许可条款得到遵守（均为 MIT/BSD/Apache 等宽松许可，
允许自由使用与再分发）。