# SuperKnobLite 智能旋钮

基于 ESP32-PICO-D4 的三相无刷 FOC 触觉旋钮，配套一套在原厂固件基础上扩展的远程控制能力（电机触觉模式、位置感知、转速伺服）与一个纯前端串口控制台。

[English](README.md) · [技术方案](docs/技术方案.md) · [项目说明](AGENTS.md)

## 目录结构

```
SuperKnobLite/
├── superknob_control.html        ★ 串口控制台（单文件网页，Web Serial API）
├── tools/
│   ├── paced_esptool.py          烧录包装：分片节流写入，规避 CH343 CDC 丢包
│   └── knob_server.mjs           电脑资源采集服务（Node，零依赖）
├── Firmware for two knob make__/  ★ 固件主工程（推荐基线，2023-01 + 本次扩展）
├── Firmware for two knob/         旧固件工程（2022-10，与上面仅屏幕偏移差 2 行）
├── SuperKnobLite 二代旋钮电路图及pcb/  Altium 原理图/PCB（100MB+ 二进制，未入库）
├── main/                          PC 端 PyInstaller 打包产物（未入库）
├── super_knob_for_pc.py           PC 资源监控脚本（psutil → 串口 JSON）
├── AGENTS.md                      项目硬件/固件/协议说明
├── README_cn.md                   本文件
└── docs/技术方案.md               ★ 本次功能扩展的完整技术方案
```

## 快速开始

### 1. 编译固件

```bash
brew install platformio          # 已验证 espressif32@4.1.0 在 Apple Silicon 可正常编译
cd "Firmware for two knob make__"
pio run                          # 产物：.pio/build/esp32doit-devkit-v1/firmware.bin
```

### 2. 烧录固件

**必须用 paced 包装**，否则大批量写入会因 CH343 CDC 丢包而失败（详见技术方案「CDC 链路丢包」一节）：

```bash
cd "Firmware for two knob make__"
python3 ../tools/paced_esptool.py --chip esp32 -p /dev/cu.usbmodem54ED0456411 \
  -b 115200 --no-stub write-flash 0x10000 .pio/build/esp32doit-devkit-v1/firmware.bin
```

> Windows/Linux 下把端口换成 `COMx` / `/dev/ttyACM0` 即可。
> 首次烧录（空白 flash）还需一并写入 bootloader、分区表、boot_app0，可用 `zeasy_upload_bin/` 目录内的现成 bin。

### 3. 启动控制台

```bash
node tools/knob_server.mjs       # 采集服务 + 页面托管 → http://127.0.0.1:8123/
```

浏览器打开 <http://127.0.0.1:8123/>（**需 Chrome / Edge**，Safari 不支持 Web Serial API）。

控制台四个标签页：

| 标签页 | 功能 |
|---|---|
| ① 连接测试 | 选择串口、自动握手（回显探测 → 复位捕获启动日志双指纹）、可靠性压测（逐包校验 + RTT 统计） |
| ② 设备自检 | 8 项自检：通信链路、固件启动、电机/FOC 任务、触摸键、屏幕数据链路、编码器、SHT30、BLE |
| 🎡 电机玩法 | **转速伺服**（安全限速调速）、**六种触觉模式**、**旋钮位置实时感知** |
| ③ 控制面板 | 电机角度环远程控制、屏幕相关功能（折叠）、串口曲线、事件流 |
| ④ 断开连接 | 会话统计、断前复位、会话总结 |

## 电机玩法（本项目核心扩展）

**六种触觉模式**（一键切换，手感各异）：

| 模式 | 指令 | 手感 |
|---|---|---|
| 档位/棘轮 | `{"t":"haptic","cmd":"set","mode":"detent"}` | 一格一格的机械档位感 |
| 弹簧回中 | `...,"mode":"spring"` | 越转越重，松手回弹 |
| 粘滞搅拌 | `...,"mode":"viscous"` | 阻尼随转速增大，停即静止 |
| 单向棘轮 | `...,"mode":"oneway"}` | 只许正转，反转被顶住 |
| 离心限位 | `...,"mode":"endstop","range":3.0}` | 范围内自由，两端硬限位墙 |
| 自由飞轮 | `...,"mode":"free"}` | 拨一下滑行好几圈，几乎无阻尼 |

**转速伺服**（安全范围内的最大转速可调）：

```json
{"t":"haptic","cmd":"velservo","enable":true,"speed":8.0,"max":15.0}
```

速度闭环控制（`MotionControlType::velocity` + PID），`speed` 为负即反转。安全限速：`max` 与目标值双重钳制在 ±20 rad/s（约 ±191 rpm），另加电压限幅与防飞车保护。

**位置感知**（50ms 上报一次）：

```json
{"t":"haptic","cmd":"report","enable":true,"interval":50}
// 设备持续上报：{"t":"pos","angle":6.469,"position":0,"velocity":0.098,"haptic":0,"remote":0}
```

页面侧对 `angle` 做角度解缠绕（跨 2π 边界累加），得到真实累计圈数。

## 串口协议

设备 UART0 @115200，行分隔 JSON。**用 `t` 字段区分新旧协议**，旧版 `{"cpu":x,"memory":y}` 完全兼容。

| 方向 | 报文 | 说明 |
|---|---|---|
| PC→设备 | `{"cpu":16.7,"memory":90.7}` | 驱动「电脑监测」页（原厂协议） |
| 设备→PC | `16.70` / `90.70` | 原厂回显，用于心跳校验 |
| PC→设备 | `{"t":"motor",...}` | 电机角度环远程控制（hold/rotate/sweep/step/release） |
| PC→设备 | `{"t":"haptic",...}` | 触觉模式切换 / 转速伺服 / 位置上报开关 |
| PC→设备 | `{"t":"sys",...}` | ping / status / display_test / display_diag |
| 设备→PC | `{"t":"ack","cmd":...,"ok":bool}` | 每条指令的应答 |
| 设备→PC | `{"t":"pos",...}` | 位置/角度/速度周期上报 |
| 设备→PC | `{"t":"status",...}` | 电机与伺服状态查询 |

## 已知问题

1. **本机屏幕模组已损坏**——ST7735 寄存器回读全 0xFF、绕过 LVGL 直接纯色刷屏亦无显示，判定为面板/排线硬件故障。屏幕相关功能已收进页面折叠区，不影响其余功能。
2. **原厂固件缺陷**：JSON 解析失败后不清空接收缓冲，会导致串口永久失步（须复位设备恢复）。页面已对该状态给出醒���提示。
3. **CDC 链路方向性丢包**：PC→设备大包会丢，设备→PC 完好。所有批量写入必须走 paced 包装。
4. **烧录需 `--no-stub`**：CDC 模式下 stub 下载器上传必然校验失败，ROM loader 模式可用。

## 相关文档

- [`docs/技术方案.md`](docs/技术方案.md) —— 本次功能扩展的完整技术方案（协议设计、FOC 控制架构、踩坑记录）
- [`AGENTS.md`](AGENTS.md) —— 项目硬件、固件架构、烧录与开发环境说明
- [`LICENSE-THIRD-PARTY.md`](LICENSE-THIRD-PARTY.md) —— 第三方组件许可与来源说明

## 许可

本项目采用 [MIT 许可](LICENSE) —— 允许自由使用、修改、分发、商用与私有化，
唯一要求是保留版权声明与许可声明。

```text
Copyright (c) 2026 SuperKnobLite Contributors
Copyright (c) 2022 NXP (gui_super_knob.h 中的部分界面代码)
```

本仓库包含的第三方组件（Simple FOC、lvgl、TFT_eSPI、ArduinoJson、
ESP32 BLE Keyboard、Adafruit NeoPixel、PubSubClient 等）各自保留原有许可，
均为 MIT / BSD / Apache-2.0 等宽松条款，不受本项目许可变更影响。
详见 [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md)。