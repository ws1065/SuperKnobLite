/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-15 17:02:59
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-11 15:46:10
 */
#pragma once

#include <Arduino.h>

#define SUPER_KNOB_VERSION "2.0.0"
#define ENABLE_BLE_KEY_BOARD
#define ENABLE_WS2812_LED
//#define ESP32C3_ENABLE
//#define ENABLE_SIMPLEFOC_STUDIO


#define LVGL_RUNNING_CORE   0            //内核选择   LVGL用核心0  其他用核心1
#define ESP32_RUNNING_CORE  1            //内核选择   LVGL用核心0  其他用核心1
#define ESP32_TOUCH_PIN1    33           //
#define ESP32_TOUCH_PIN2    32           //
#define ESP32_TOUCH_PIN3    12           //
#define WS2812_PIN          11           //定义选择引脚
#define WS2812_NUM          1            //定义led个数，决定数组长度

const char tuya_cacert_pem[] = {\
"-----BEGIN CERTIFICATE-----\n"\
"MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"\
"EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"\
"EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"\
"ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"\
"NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"\
"EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"\
"AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"\
"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"\
"E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"\
"/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"\
"DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"\
"GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"\
"tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"\
"AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"\
"FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"\
"WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"\
"9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"\
"gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"\
"2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"\
"LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"\
"4uJEvlz36hz1\n"\
"-----END CERTIFICATE-----\n"};



struct _knob_message
{
    int ucMessageID;
    char ucData[100];
};

struct _ws2812_message
{
    int ws2812_status;
    int ws2812_time;
};
struct _iot_control_message
{
    int ucMessageID;
    char ucData[100];
};

struct _iot_pc_monitor
{
    double monitor_cpu;
    double monitor_momoey;

};

extern _iot_pc_monitor PC_MONITOR;
extern _knob_message LVGL_MSG;
extern _knob_message MOTOR_MSG;
extern _ws2812_message WS2812_MSG;
extern _iot_control_message IOT_CONTROL_MSG;

extern QueueHandle_t motor_msg_Queue;  //lvgl 接收消息队列
extern QueueHandle_t motor_rcv_Queue;  //motor 接收消息队列
extern QueueHandle_t ws2812_rcv_Queue;  //ws2812 接收消息队列
extern QueueHandle_t iot_control_Queue;  //ws2812 接收消息队列

