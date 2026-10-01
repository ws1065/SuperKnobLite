# SuperKnobLite 项目说明（AGENTS.md）

本文件汇总当前工作区的硬件、固件、通信协议与开发环境信息，供后续开发/重构参考。
分析日期：2026-10-01。

## 目录结构

```
SuperKnobLite/
├── Firmware for two knob/              PlatformIO 固件工程（2022-10 构建，旧）
├── Firmware for two knob make__/        同一工程更新版（2023-01，★推荐基线）
│   └── 与旧版仅 core/display.cpp 差 2 行：disp_drv.offset_x=-2; offset_y=-1（屏幕偏移修正）
├── SuperKnobLite 二代旋钮电路图及pcb/    Altium 原理图/PCB
│   ├── SuperKnoblite/                  主控板 v1.4（SuperKnobLite.pdf 为原理图）
│   └── Screen/                         屏幕子板 v1.1
├── main/                                PC 端 PyInstaller 打包程序（PyQt5 + pyserial）
└── super_knob_for_pc.py                 PC 性能监控脚本（psutil → 串口 JSON）
```

上级目录另有 `SmartKnobView/`（不同硬件，HX711 应变片按压式 SmartKnob）与 `countcode_bk/`，**与本项目硬件无关**，分析/开发时不要混用其固件。

## 硬件架构

**主控**：ESP32-PICO-D4（SiP，内置 4MB Flash，双核 240MHz）。`platformio.ini`: `board=pico32`，分区 `huge_app.csv`（3MB app + 960KB spiffs，无 OTA 双分区）。

| 功能 | GPIO | 说明 |
|---|---|---|
| FOC 三相 PWM A/B/C | 22 / 4 / 5 | → DRV8313PWPR IN1/IN2/IN3 |
| 驱动使能 nEN | 21 | DRV8313 EN1/2/3 并联 |
| AS5600 磁编码器 | SDA=19, SCL=18 | I2C0 @400kHz，地址 0x36，12bit |
| SHT30 温湿度 | 同 I2C 总线，0x44 | **默认不贴料** |
| TFT 屏 ST7735 80×160 | MOSI=13, SCLK=14, CS=15, DC=27, RST=2, BL=26 | SPI@40MHz，BGR，`ST7735_GREENTAB160x80` |
| WS2812B 氛围灯 | 11 | 1 颗，NEO_GRB |
| 电容触摸按键 | 32（主用，33/12 定义未用） | `touchRead()<15` 判定+消抖 |
| UART0（USB） | TX=1, RX=3 | → CH343P → Type-C，115200，带 DTR/RTS 自动下载电路 |
| UART1（扩展口 CON2） | TX=10, RX=9 | FPC 4P，**默认不贴料** |

**电机**：三相无刷云台电机，7 极对（`BLDCMotor(7)`），纯电压闭环 FOC（无电流采样）。供电 12V（MP2460 buck 产生 3.3V）。

## 固件架构

PlatformIO + Arduino 框架，核心依赖：`SimpleFOC 2.2.1`、`lvgl 8.2.0`、`TFT_eSPI`、`ESP32 BLE Keyboard`、`Adafruit NeoPixel`（`PubSubClient`/tuya 证书为未使用的残留代码，可删除）。

双核 FreeRTOS 任务划分：
```
Core1: Task_foc(pri2，1kHz FOC 控制环)
Core0: Task_lvgl(pri3，UI 渲染+编码器/触摸输入)
Core1: Task_module(pri4，消息转发) / Task_ws2812(pri4，灯效)
```
通过队列解耦：`motor_rcv_Queue`（LVGL→电机）、`motor_msg_Queue`（电机→LVGL）、`ws2812_rcv_Queue`、`iot_control_Queue`。

**触觉引擎**（`src/core/motor.cpp` `Task_foc`，SmartKnob 同款 detent 算法）：
- `KnobConfig` 结构体：num_positions / position_width_radians / detent_strength_unit / endstop_strength_unit / snap_point / motor_auto
- 预设表 `super_knob_configs[6]`：UNLIMITED(1°无阻尼) / RATCHET(8.23°强阻尼) / MECHANICAL(3°,0.8阻尼) / UNDAMPED(1°,256档) / DOUBLE(2档60°模拟开关) / MOTOR_AUTO(angle 控制自转/指针模式)
- 关键保护：怠速中心点慢修正（500ms 延迟，α=0.0005）、死区（20%或1°取小）、出界端点刚度放大、|速度|>60rad/s 时力矩清零防飞车

**页面系统**（`src/user_app/gui_super_knob.h` `SUPER_KNOB_PAGE_NUM`，15 页）：
欢迎 / 主菜单 / 指针 / 传感器 / 电脑监测 / 灯带 / 风扇 / 音量 / CV复制粘贴 / 关于 / 设置 / 角度 / CPU监测 / 鲲鲲 / 一步启动。
交互机制：旋钮旋转 → `motor_config.position` 变化 → LVGL encoder 差量驱动焦点滚动；触摸 GPIO32 → 确认/退出 + 触发 WS2812 灯效；进入页面时调用 `update_motor_config(模式)` 切换触觉反馈并 `motor_shake(2,2)` 震动反馈。

**BLE HID**（`driver/ble_keyboard.cpp`）：设备名 `Super Knob pro`。映射：媒体播放/暂停、上一曲/下一曲、音量加/减、`Ctrl+C/V`、`GUI+Tab`，以及 one_step 页的 Win 组合键（`keyboard_write_common`）。

**串口协议**（UART0 115200）：PC → 旋钮发送 `{"cpu":16.7,"memory":90.7}\n`（`main.cpp:107` 用 ArduinoJson 解析，驱动 CPU 监测页指针）。`tuya_control.cpp` 仅做消息转发（Serial/Serial1），**项目中无实际 WiFi/MQTT 功能**。

## PC 端工具

- `super_knob_for_pc.py` / `main/`（PyInstaller 打包版）：PyQt5 + pyserial + psutil，串口固定 `COM8@115200`，每秒发送一次 CPU/内存 JSON。
- `zpcmonitor_python/`：与根目录脚本内容一致（md5 相同）的副本。

## 开发环境与烧录

**烧录（预编译 bin，位于 `Firmware for two knob/zeasy_upload_bin/`）**：
| 文件 | 地址 |
|---|---|
| bootloader_dio_40m.bin | 0x1000 |
| partitions.bin | 0x8000 |
| boot_app0.bin | 0xe000 |
| firmware.bin | 0x10000 |

macOS 命令：
```bash
pipx install esptool
esptool.py --chip esp32 -p /dev/cu.wchusbserial* -b 921600 write_flash \
  0x1000 bootloader_dio_40m.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
```
板载 CH343P 自动下载电路，无需手动按键；需安装 WCH CH34x macOS 驱动才能看到串口设备。

**源码编译**（本机未装 PlatformIO）：
```bash
brew install platformio   # 或 pipx install platformio
cd "Firmware for two knob make__"   # 以此为基线，比旧版多屏幕偏移修正
pio run
pio run -t upload && pio device monitor -b 115200
```

**已知坑**：
1. `espressif32@4.1.0` 为 2021 年老平台，Apple Silicon 上下载/编译可能有兼容问题，可考虑升级到 `espressif32@6.x`（SimpleFOC/LVGL/TFT_eSPI/BLE Keyboard 均兼容，需少量 API 微调）。
2. `TFT_eSPI` 的 `User_Setup.h` 与 LVGL 的 `lv_conf.h` 是直接改在 `.pio/libdeps/` 库目录内的，备份存于 `Firmware for two knob/zlibrary_readme/`；`pio run -t clean` 或重建 libdeps 会丢失这两个文件，**必须纳入版本管理**（如用 `build_flags`/`-I` 注入或 `extra_scripts` 自动拷贝）。
3. LVGL 内存池仅 32KB（`lv_conf.h` 注释说明：BLE 与 LVGL 同时开启时被迫从 64KB 降到 32KB），重构时需留意字体资源占用。
4. 当前固件已 2.04MB，接近 3MB app 分区上限，若要做 OTA 需裁剪到 <1.9MB（双分区）。

**本机环境**：无 PlatformIO/pio CLI；已有 Python 3.14、Node v24.18.0；编辑器可用 Qoder/Antigravity（VS Code 系），装 `platformio.platformio-ide` 扩展即可图形化编译烧录。

## 重新编码方向（设计要点，供后续实施参考）

- 分层：`app/`（页面业务）、`services/`（BLE HID、串口协议、LED、NVS 配置）、`haptic/`（触觉引擎+电机/编码器 HAL，Core1 @1kHz）、`drivers/`（SimpleFOC/TFT_eSPI/LVGL/NeoPixel）。
- 触觉引擎建议封装为 `HapticConfig` 结构体+运行时可调+落 NVS，预设模式复用现有 6 种数学逻辑。
- 事件总线替代裸全局队列：`KnobEvent{ROTATED/PRESSED/LONG_PRESSED/MODE_CHANGED}`、`UiCommand{SET_MODE/SHAKE/SET_ANGLE/SHOW_MSG}`。
- 交互优化建议：短按=确认、长按=返回主页（现固件短按即退出页面，容易误触）。
- 串口协议建议升级为带 `"t"` 字段的 JSON（兼容旧 `{"cpu","memory"}` 格式），新增配置/事件上报类型。
- 清理死代码：PubSubClient、tuya 证书、未使用的触摸引脚定义（33/12）。

## 相关但不属于本项目的硬件

`SmartKnobView/`（上级目录）是另一台 SmartKnob（HX711 应变片按压检测，电机 7 极对但校准方式不同，见 `按压检测不正常的解决方式.docx` 和 `过了限位电机抖动、旋转的解决办法版.docx`）。若实物硬件是这一台，需要切换到该目录下的固件而非 SuperKnobLite 的工程。
