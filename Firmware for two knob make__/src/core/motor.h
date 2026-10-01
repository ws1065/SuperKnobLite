/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-22 05:30:20
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-08 22:49:45
 */
#pragma once
#include <Arduino.h>

#define pi 3.1415926
#define init_smooth 1000 // 该值越大，初始化越慢。以防受到干扰。
#define volt_limit 8.0000

struct KnobConfig {
    //可以运动的个数
    int32_t num_positions;        
    //位置
    int32_t position;             
    //位置宽度弧度 或者是每一步的度数
    float position_width_radians; 
    //制动强度
    float detent_strength_unit;  
    //end stop强度 
    float endstop_strength_unit;  
    //中断点 
    float snap_point; 
    //使能电机自动运转
    bool motor_auto;
};
extern KnobConfig motor_config;

typedef enum
{
    CONGIF_UNLIMITED,
    CONFIG_RATCHET,
    CONFIG_MECHANICAL,
    CONFIG_UNDAMPED,
    CONFIG_DOUBLE,
    CONFIG_MPTOR_AUTO,
} MOTOR_CONFIG_MODE_E;

typedef enum
{
    MOTOR_INIT,
    MOTOR_INIT_SUCCESS,
    MOTOR_INIT_END,
    DEV_WORK_MODE,
    DEV_BLE_WORK,

} MOTOR_RUNNING_MODE_E;

int get_motor_position(void);
int get_motor_angle(void);
int set_motor_angle(float angle);
void update_motor_config(int status);
void Task_foc(void *pvParameters);

/* ============ 串口远程控制（PC 通过 JSON 指令驱动电机） ============ */
typedef enum
{
    REMOTE_OFF = 0,   // 交还给正常触觉（detent）模式
    REMOTE_HOLD,      // 保持到指定绝对角度
    REMOTE_ROTATE,    // 连续旋转：角速度 p1(rad/s)，方向 p2(±1)
    REMOTE_SWEEP,     // 往复摆动：幅度 p1(rad)，速度 p2(rad/s)
    REMOTE_STEP,      // 跳到第 p3 个档位（复用 detent 角度栅格）
} REMOTE_MODE_E;

void motor_remote_set(REMOTE_MODE_E mode, float p1, float p2, int32_t p3);
void motor_remote_stop(void);
uint8_t motor_remote_mode(void);
float motor_remote_target(void);
float motor_remote_velocity(void);

/* ============ 远程可切换的触觉模式 ============ */
typedef enum
{
    HAPTIC_DETENT = 0,  // 档位/棘轮 = 现有 super_knob_configs 预设（默认，保持原行为）
    HAPTIC_SPRING,      // 弹簧回中：偏离中心越远回弹力越大
    HAPTIC_VISCOUS,     // 粘滞搅拌：阻尼只与转速成正比，松手停在原地
    HAPTIC_ONEWAY,      // 单向棘轮：允许正转，反转被强力阻止
    HAPTIC_ENDSTOP,     // 离心限位：范围内自由旋转，两端硬限位墙
    HAPTIC_FREE = 5,    // 自由飞轮：几乎无阻尼，拨一下能连续转好几圈才停
} HAPTIC_MODE_E;

// 切换触觉模式。param 为模式附加参数（HAPTIC_ENDSTOP 有效，0 表示用默认值）
void haptic_set_mode(uint8_t mode, float param);
uint8_t haptic_get_mode(void);

// 旋钮位置/角度周期上报（默认 20Hz，输出单行 JSON）
void haptic_report_enable(bool on, uint16_t interval_ms);
bool haptic_report_active(void);

/* ============ 转速伺服（速度闭环，独立于触觉模式） ============ */
void vel_servo_enable(bool on, float target_rad_s, float max_rad_s);
bool vel_servo_active(void);
float vel_servo_target(void);
float vel_servo_max_current(void);