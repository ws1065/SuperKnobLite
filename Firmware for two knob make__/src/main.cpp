/*
 * @Descripttion:
 * @version:
 * @Author: congsir
 * @Date: 2022-05-14 23:55:57
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-08 00:02:16
 */
// https://docs.simplefoc.com/bldcmotor

#include <main.h>
#include <core/display.h>
#include <core/motor.h>
#include <driver/tuya_control.h>
#ifdef ENABLE_BLE_KEY_BOARD
#include <driver/ble_keyboard.h>
#endif
#include <driver/ws2812_driver.h>
#include "driver/driver_sht30.h"
#include <ArduinoJson.h>

TaskHandle_t Task_foc_Handle;    // foc 任务
TaskHandle_t Task_lvgl_Handle;   // lvgl 任务
TaskHandle_t Task_module_Handle; //子模块通讯任务
TaskHandle_t Task_ws2812_Handle; // ws2812 任务

QueueHandle_t motor_msg_Queue;   // lvgl 接收消息队列
QueueHandle_t motor_rcv_Queue;   // motor 接收消息队列
QueueHandle_t ws2812_rcv_Queue;  // ws2812 接收消息队列
QueueHandle_t iot_control_Queue; // ws2812 接收消息队列

_knob_message LVGL_MSG;
_knob_message MOTOR_MSG;
_ws2812_message WS2812_MSG;
_iot_control_message IOT_CONTROL_MSG;
_iot_pc_monitor PC_MONITOR;

StaticJsonDocument<200> doc;

void super_knob_print_logo(void);
void serialReceiveUserCommand(void);
static void handle_extended_command(void);
void setup()
{
    Serial.begin(115200);
    super_knob_print_logo();
    Serial1.begin(115200);
#ifdef ENABLE_BLE_KEY_BOARD
    ble_keyboard_init();
#endif
    motor_msg_Queue = xQueueCreate(10, sizeof(_knob_message *));
    motor_rcv_Queue = xQueueCreate(10, sizeof(_knob_message *));
    ws2812_rcv_Queue = xQueueCreate(10, sizeof(_ws2812_message *));
    iot_control_Queue = xQueueCreate(10, sizeof(_ws2812_message *));

    WS2812_init();
    driver_sht30_init();

#ifdef ESP32C3_ENABLE
    xTaskCreate(Task_foc, "Task_foc", 4096, NULL, 2, &Task_foc_Handle);
    xTaskCreate(Task_lvgl, "Task_lvgl", 4096, NULL, 3, &Task_lvgl_Handle);
    xTaskCreate(Task_module, "Task_knob", 2048, NULL, 4, &Task_module_Handle);
    xTaskCreate(Task_ws2812, "Task_ws2812", 1024, NULL, 4, &Task_ws2812_Handle);
#else
    xTaskCreatePinnedToCore(
        Task_foc, "Task_foc", 4096, NULL, 2, &Task_foc_Handle, ESP32_RUNNING_CORE);
    xTaskCreatePinnedToCore(
        Task_lvgl, "Task_lvgl", 4096, NULL, 3, &Task_lvgl_Handle, LVGL_RUNNING_CORE);
    xTaskCreatePinnedToCore(
        Task_module, "Task_knob", 2048, NULL, 4, &Task_module_Handle, ESP32_RUNNING_CORE);
    xTaskCreatePinnedToCore(
        Task_ws2812, "Task_ws2812", 2048, NULL, 4, &Task_ws2812_Handle, ESP32_RUNNING_CORE);
#endif
}

void loop()
{
    delay(100);
    serialReceiveUserCommand();
}

void super_knob_print_logo(void)
{
    Serial.println("\r\n\r\n");
    Serial.println("         _          _ _                                     _                _       ");
    Serial.println("        | |__   ___| | | ___    ___ _   _ _ __   ___ _ __  | | ___ __   ___ | |__    ");
    Serial.println("        | '_ \\ / _ \\ | |/ _ \\  / __| | | | '_ \\ / _ \\ '__| | |/ / '_ \\ / _ \\| '_ \\");
    Serial.println("        | | | |  __/ | | (_) | \\__ \\ |_| | |_) |  __/ |    |   <| | | | (_) | |_) |   ");
    Serial.println("        |_| |_|\\___|_|_|\\___/  |___/\\__,_| .__/ \\___|_|    |_|\\_\\_| |_|\\___/|_.__/    ");
    Serial.println("                                         |_|                                          ");
}

// 扩展串口协议指令处理：按 "t" 字段分发，所有指令统一回一条 ack
// {"t":"motor","cmd":"hold","angle":0.5} {"t":"sys","cmd":"ping"} 等
static void handle_extended_command(void)
{
    const char *type = doc["t"] | ""; // 指令类别：motor / sys
    const char *cmd = doc["cmd"] | ""; // 具体指令名，未知时为空
    bool handled = false;             // 是否命中已知指令

    //判断是不是电机类指令
    if (strcmp(type, "motor") == 0)
    {
        // 保持到指定绝对角度
        if (strcmp(cmd, "hold") == 0)
        {
            float angle = doc["angle"] | 0.0f;
            motor_remote_set(REMOTE_HOLD, angle, 0.0f, 0);
            handled = true;
        }
        // 连续旋转：角速度 + 方向
        else if (strcmp(cmd, "rotate") == 0)
        {
            float speed = doc["speed"] | 0.0f;
            float dir = doc["dir"] | 1.0f;
            dir = (dir < 0.0f) ? -1.0f : 1.0f; // 方向只取 ±1
            motor_remote_set(REMOTE_ROTATE, speed, dir, 0);
            handled = true;
        }
        // 往复摆动：幅度 + 速度
        else if (strcmp(cmd, "sweep") == 0)
        {
            float range = doc["range"] | 0.0f;
            float speed = doc["speed"] | 0.0f;
            motor_remote_set(REMOTE_SWEEP, range, speed, 0);
            handled = true;
        }
        // 跳到指定档位
        else if (strcmp(cmd, "step") == 0)
        {
            int32_t pos = doc["pos"] | 0;
            motor_remote_set(REMOTE_STEP, 0.0f, 0.0f, pos);
            handled = true;
        }
        // 交还给正常触觉模式
        else if (strcmp(cmd, "release") == 0)
        {
            motor_remote_stop();
            handled = true;
        }
    }
    //判断是不是触觉类指令
    else if (strcmp(type, "haptic") == 0)
    {
        // 切换触觉模式
        if (strcmp(cmd, "set") == 0)
        {
            const char *mode = doc["mode"] | "";
            if (strcmp(mode, "detent") == 0)
            {
                haptic_set_mode(HAPTIC_DETENT, 0);
                handled = true;
            }
            else if (strcmp(mode, "spring") == 0)
            {
                haptic_set_mode(HAPTIC_SPRING, 0);
                handled = true;
            }
            else if (strcmp(mode, "viscous") == 0)
            {
                haptic_set_mode(HAPTIC_VISCOUS, 0);
                handled = true;
            }
            else if (strcmp(mode, "oneway") == 0)
            {
                haptic_set_mode(HAPTIC_ONEWAY, 0);
                handled = true;
            }
            else if (strcmp(mode, "endstop") == 0)
            {
                // range 是范围半径(rad)，缺省 3.0
                float range = doc["range"] | 3.0f;
                if (range <= 0) range = 3.0f;
                haptic_set_mode(HAPTIC_ENDSTOP, range);
                handled = true;
            }
            else if (strcmp(mode, "free") == 0)
            {
                // 自由飞轮：几乎没有阻尼，拨一下转好几圈
                haptic_set_mode(HAPTIC_FREE, 0);
                handled = true;
            }
            // 未知 mode 不置 handled，最终回 ok:false
        }
        // 转速伺服开关：速度闭环，独立于触觉模式
        else if (strcmp(cmd, "velservo") == 0)
        {
            bool on = doc["enable"] | false;
            if (on)
            {
                // 调速时 max 可缺省，沿用当前生效值
                float max_v = doc["max"] | vel_servo_max_current();
                // speed 也可缺省，沿用当前目标值
                float speed = doc["speed"] | vel_servo_target();
                vel_servo_enable(true, speed, max_v);
            }
            else
            {
                vel_servo_enable(false, 0, 0);
            }
            handled = true;
        }
        // 位置上报开关
        else if (strcmp(cmd, "report") == 0)
        {
            bool on = doc["enable"] | false;
            uint16_t interval = (uint16_t)(doc["interval"] | 50);
            haptic_report_enable(on, interval);
            handled = true;
        }
    }
    //判断是不是系统类指令
    else if (strcmp(type, "sys") == 0)
    {
        // 联通性测试
        if (strcmp(cmd, "ping") == 0)
        {
            Serial.println("{\"t\":\"pong\"}");
            handled = true;
        }
        // 上报当前电机远程状态与 PC 监测数据
        else if (strcmp(cmd, "status") == 0)
        {
            char status_buf[256];
            snprintf(status_buf, sizeof(status_buf),
                     "{\"t\":\"status\",\"remote\":%s,\"mode\":%u,\"angle\":%.2f,\"velocity\":%.2f,\"cpu\":%.2f,\"memory\":%.2f,\"servo\":%s,\"servo_target\":%.2f}",
                     (motor_remote_mode() != REMOTE_OFF) ? "true" : "false",
                     (unsigned)motor_remote_mode(),
                     motor_remote_target(),
                     motor_remote_velocity(),
                     PC_MONITOR.monitor_cpu,
                     PC_MONITOR.monitor_momoey,
                     vel_servo_active() ? "true" : "false",
                     vel_servo_target());
            Serial.println(status_buf);
            handled = true;
        }
        // 屏幕自检（纯色/刷页测试）
        else if (strcmp(cmd, "display_test") == 0)
        {
            const char *mode = doc["mode"] | "";
            if (strcmp(mode, "start") == 0)
            {
                display_self_test_start();
                handled = true;
            }
            else if (strcmp(mode, "stop") == 0)
            {
                display_self_test_stop();
                handled = true;
            }
        }
        // 读取屏幕模组诊断信息
        else if (strcmp(cmd, "display_diag") == 0)
        {
            display_read_panel_diag();
            handled = true;
        }
    }

    // 每条指令都回一条 ack，未知指令回 ok:false
    if (handled)
    {
        Serial.printf("{\"t\":\"ack\",\"cmd\":\"%s\",\"ok\":true}\n", cmd);
    }
    else
    {
        Serial.printf("{\"t\":\"ack\",\"cmd\":\"%s\",\"ok\":false}\n", cmd);
    }
}

void serialReceiveUserCommand(void)
{

    // a string to hold incoming data
    static String received_chars;
    while (Serial.available())
    {
        char inChar = (char)Serial.read();
        // add it to the string buffer:
        received_chars += inChar;
        // end of user input
        if (inChar == '\n')
        {
            //Serial.println(received_chars);
            //{"cpu":16.7,"memory":90.7}
            DeserializationError error = deserializeJson(doc, received_chars);
            // Test if parsing succeeds.
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            // 带 "t" 字符串字段的走扩展指令协议，否则按原来的 cpu/memory 协议处理
            if (doc["t"].is<const char*>())
            {
                handle_extended_command();
            }
            else
            {
                PC_MONITOR.monitor_cpu = doc["cpu"];
                PC_MONITOR.monitor_momoey = doc["memory"];

                Serial.println(PC_MONITOR.monitor_cpu);
                Serial.println(PC_MONITOR.monitor_momoey);
            }

            // reset the command buffer
            received_chars = "";
        }
    }
}
