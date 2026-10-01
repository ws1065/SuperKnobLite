/*
 * @Descripttion:
 * @version:
 * @Author: congsir
 * @Date: 2022-05-22 05:30:09
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-08 23:01:05
 */
#include "motor.h"
#include <main.h>
#include <SimpleFOC.h>
#include "display.h"
#include "driver/ble_keyboard.h"

static KnobConfig super_knob_configs[] = {
    {
        0,
        0,
        10 * PI / 180,
        0,
        1,
        1.1,
        false, //无限制  不制动
    },
    {
        0,
        0,
        8.225806452 * PI / 180,
        2,
        1,
        1.1,
        false, //粗糙的棘轮 强阻尼
    },
    {
        0,
        0,
        3 * PI / 180,
        0.8,
        1,
        2,
        false, //任意运动的控制  有阻尼 类似于机械旋钮
    },
    {
        256,
        127,
        1 * PI / 180,
        0,
        1,
        1.1,
        false, //任意运动的控制  无阻尼
    },
    {
        2, //可以运动的个数
        0,
        60 * PI / 180, //每一步60度
        1,             //制动强度为1
        1,
        0.55,                    // Note the snap point is slightly past the midpoint (0.5); compare to normal detents which use a snap point *past* the next value (i.e. > 1)
        false, //模拟开关  强制动
    },
    {
        0,
        0,
        0,
        0,
        0,
        0,
        true,
    },
};

//死区制动百分率
static const float DEAD_ZONE_DETENT_PERCENT = 0.2;
//死区RAD?
static const float DEAD_ZONE_RAD = 1 * _PI / 180;

//怠速速度ewma alpha
static const float IDLE_VELOCITY_EWMA_ALPHA = 0.001;
//怠速速度每秒钟弧度
static const float IDLE_VELOCITY_RAD_PER_SEC = 0.05;
//怠速修正延迟millis
static const uint32_t IDLE_CORRECTION_DELAY_MILLIS = 500;
//怠速校正最大角度rad
static const float IDLE_CORRECTION_MAX_ANGLE_RAD = 5 * PI / 180;
//怠速修正率
static const float IDLE_CORRECTION_RATE_ALPHA = 0.0005;

//先设置一个默认值，等消息队列发送状态过来
KnobConfig motor_config = {
    .num_positions = 32,
    .position = 0,
    .position_width_radians = 8.225806452 * _PI / 180,
    .detent_strength_unit = 2.3,
    .endstop_strength_unit = 1,
    .snap_point = 1.1,
};

//设置位置传感器（编码器、磁传感器、霍尔传感器）
// MagneticSensorI2C sensor = MagneticSensorI2C(0x36, 12, 0x0E, 4);
MagneticSensorI2C sensor = MagneticSensorI2C(AS5600_I2C);
TwoWire I2Cone = TwoWire(0);
//设置电机(极对数)
BLDCMotor motor = BLDCMotor(7);
//设置驱动器 （无刷直流电机、步进电机）
// BLDCDriver3PWM( pin_pwmA, pin_pwmB, pin_pwmC, enable使能引脚（可选的）)
// BLDCDriver3PWM driver = BLDCDriver3PWM(26, 27, 14, 12);
BLDCDriver3PWM driver = BLDCDriver3PWM(22, 4, 5, 21);

//设置电流检测 (如果不支持、可以跳过)
//在线电流检测 InlineCurrentSense。低侧电流检测 LowsideCurrentSense。
// InlineCurrentSense current_sense = InlineCurrentSense(0.01, 50, A0, A2);

//设置控制命令接口 关闭输出 可有效防止抖动
float target_velocity = 0; 
#ifdef ENABLE_SIMPLEFOC_STUDIO
Commander command = Commander(Serial);
void doTarget(char* cmd) { command.scalar(&target_velocity, cmd); }    // 串口控制指令：目标值
void onMotor(char *cmd) { command.motor(&motor, cmd); }
#endif



//开机初始化角度至0
void init_angle(void)
{
    float target_angle = 0;
    target_angle = sensor.getAngle();
    float delta = volt_limit / init_smooth;
    for (int i = 0; i <= init_smooth; i++)
    {
        motor.voltage_limit = delta * i;
        motor.loopFOC();
        motor.move(target_angle);
    }
    motor.voltage_limit = volt_limit;
}

float CLAMP(const float value, const float low, const float high)
{
    return value < low ? low : (value > high ? high : value);
}

/* ============ 串口远程电机控制 ============ */
static volatile uint8_t g_remote_mode = REMOTE_OFF;
static volatile float g_remote_p1 = 0;
static volatile float g_remote_p2 = 0;
static volatile int32_t g_remote_p3 = 0;
static float g_remote_target = 0;    // 仅 Task_foc 内部使用
static float g_remote_phase = 0;     // 扫掠相位
static float g_remote_base = 0;      // 进入远程模式时的角度基准
static uint32_t g_remote_last_ms = 0;

void motor_remote_set(REMOTE_MODE_E mode, float p1, float p2, int32_t p3)
{
    // 参数先写、模式后写：Task_foc 优先级更高，可能在任意点抢占
    g_remote_p1 = p1;
    g_remote_p2 = p2;
    g_remote_p3 = p3;
    g_remote_mode = (uint8_t)mode;
}

void motor_remote_stop(void)
{
    g_remote_mode = REMOTE_OFF;
    // 恢复 detent 模式的力矩环参数
    motor.controller = MotionControlType::torque;
    motor.PID_velocity.P = 0.08;
    motor.PID_velocity.I = 4;
    motor.PID_velocity.D = 0.0002;
    motor.LPF_velocity.Tf = 0.02;
    motor.P_angle.P = 20;
    motor.velocity_limit = 5;
}

uint8_t motor_remote_mode(void) { return g_remote_mode; }
float motor_remote_target(void) { return g_remote_target; }
float motor_remote_velocity(void) { return motor.shaft_velocity; }

/* ============ 远程可切换的触觉模式 ============ */
// 参数先写、模式后写：Task_foc 优先级更高，可能在任意点抢占
static volatile uint8_t g_haptic_mode = HAPTIC_DETENT;
static volatile float g_haptic_param = 0;      // 模式附加参数
static volatile bool g_haptic_center_valid = false;
static float g_haptic_center = 0;             // 进入该模式时的角度零点，仅 Task_foc 使用

//端stop模式的默认范围半径(rad)
static const float HAPTIC_ENDSTOP_DEFAULT_RANGE = 3.0f;
//力矩上限，防止大力矩把电机打飞
static const float HAPTIC_TORQUE_LIMIT = 10.0f;
//各模式力矩系数
static const float HAPTIC_SPRING_K = -2.5f;     // 弹簧回中：力矩 = K * 偏离中心角度
static const float HAPTIC_VISCOUS_K = -0.15f;   // 粘滞阻尼：力矩 = K * 转速
static const float HAPTIC_VISCOUS_LIMIT = 6.0f;
static const float HAPTIC_ONEWAY_FREE_K = -0.02f;  // 正转时几乎自由
static const float HAPTIC_ONEWAY_BLOCK_K = 2.0f;   // 反转时强力阻止
static const float HAPTIC_ONEWAY_LIMIT = 8.0f;
static const float HAPTIC_ONEWAY_DEAD_VEL = -0.05f; // 反向抖动阈值 rad/s
static const float HAPTIC_ENDSTOP_SOFT_K = -0.1f;   // 范围内轻微回中
static const float HAPTIC_ENDSTOP_HARD_K = -8.0f;   // 越界后的硬墙
//自由飞轮：极小的粘性阻尼系数。太小会像永动机一样一直转，太大就拨不动了。
//这个值让转速衰减时间常数约为 1/0.004 = 250s，手感上是"拨一下转好几圈慢慢停"
static const float HAPTIC_FREE_K = -0.004f;
//自由飞轮力矩上限（防飞车时的兜底）
static const float HAPTIC_FREE_LIMIT = 0.5f;
//自由飞轮惯性初值（进入模式时的转速，仅用于记录/调试）
static volatile float g_free_spin_vel = 0;

/* ============ 转速伺服（速度闭环） ============ */
// 参数先写、开关后写：Task_foc 优先级更高，可能在任意点抢占
static volatile bool g_vel_servo_on = false;
static volatile float g_vel_servo_target = 0;    // 目标转速 rad/s（负数为反转）
static volatile float g_vel_servo_max = 15.0f;    // 硬安全限速 rad/s
//max 自身的安全上下限
static const float VEL_SERVO_MAX_LIMIT = 20.0f;  // 20 rad/s ≈ 191rpm，再高不安全
static const float VEL_SERVO_MAX_FLOOR = 1.0f;
//速度环 PID 与驱动电压上限
static const float VEL_SERVO_PID_P = 0.6f;
static const float VEL_SERVO_PID_I = 1.5f;
static const float VEL_SERVO_PID_D = 0.0f;
static const float VEL_SERVO_VOLTAGE_LIMIT = 8.0f;

void vel_servo_enable(bool on, float target_rad_s, float max_rad_s)
{
    if (!on)
    {
        g_vel_servo_on = false;
        // 恢复触觉模式的力矩环参数
        motor.controller = MotionControlType::torque;
        motor.PID_velocity.P = 0.08;
        motor.PID_velocity.I = 4;
        motor.PID_velocity.D = 0.0002;
        motor.LPF_velocity.Tf = 0.02;
        // 回到档位模式，交还控制权
        haptic_set_mode(HAPTIC_DETENT, 0);
        return;
    }

    // 硬安全限速：max 自身钳制在 [1, 20] rad/s
    float max_v = CLAMP(max_rad_s, VEL_SERVO_MAX_FLOOR, VEL_SERVO_MAX_LIMIT);
    // 目标转速也钳制在 ±max
    float tgt = CLAMP(target_rad_s, -max_v, max_v);

    // 参数先写、开关后写
    g_vel_servo_max = max_v;
    g_vel_servo_target = tgt;

    // 切换为速度控制
    motor.controller = MotionControlType::velocity;
    motor.PID_velocity.P = VEL_SERVO_PID_P;
    motor.PID_velocity.I = VEL_SERVO_PID_I;
    motor.PID_velocity.D = VEL_SERVO_PID_D;
    motor.voltage_limit = VEL_SERVO_VOLTAGE_LIMIT; // 限驱动电压，防大电流

    g_vel_servo_on = true;
}

bool vel_servo_active(void) { return g_vel_servo_on; }
float vel_servo_target(void) { return g_vel_servo_target; }
float vel_servo_max_current(void) { return g_vel_servo_max; }

//位置上报
static volatile bool g_report_on = false;
static volatile uint16_t g_report_interval_ms = 50;
static uint32_t g_report_last_ms = 0;
static bool g_report_active = false;

//切换触觉模式
void haptic_set_mode(uint8_t mode, float param)
{
    if (mode > HAPTIC_FREE)
    {
        mode = HAPTIC_DETENT;   // 未知模式回落到档位模式
    }
    // 参数先写、模式后写：Task_foc 可能在任意点抢占
    g_haptic_param = param;
    g_haptic_center_valid = false; // 让 Task_foc 在下一圈重新捕获零点
    g_haptic_mode = mode;

    // 恢复 detent 用的力矩环参数，各模式共用
    motor.controller = MotionControlType::torque;
    motor.PID_velocity.P = 0.08;
    motor.PID_velocity.I = 4;
    motor.PID_velocity.D = 0.0002;
    motor.LPF_velocity.Tf = 0.02;
    motor.P_angle.P = 20;
    motor.velocity_limit = 5;

    if (mode == HAPTIC_DETENT)
    {
        // 回到档位/棘轮：恢复默认预设
        update_motor_config(CONFIG_MECHANICAL);
    }
}

uint8_t haptic_get_mode(void) { return g_haptic_mode; }

//开启/关闭位置上报
void haptic_report_enable(bool on, uint16_t interval_ms)
{
    // 限幅，防止间隔过密把串口刷爆
    if (interval_ms < 10) interval_ms = 10;
    g_report_interval_ms = interval_ms;
    g_report_on = on;
    if (!on)
    {
        g_report_active = false;
    }
}

bool haptic_report_active(void) { return g_report_active; }

void motor_shake(int strength, int delay_time)
{
    motor.move(strength);
    for (int i = 0; i < delay_time; i++)
    {
        motor.loopFOC();
        vTaskDelay(1);
    }
    motor.move(-strength);
    for (int i = 0; i < delay_time; i++)
    {
        motor.loopFOC();
        vTaskDelay(1);
    }
}
int get_motor_angle(void)
{
    return sensor.getAngle();
}

int set_motor_angle(float angle)
{
    target_velocity = angle;
    return 0;
}

int get_motor_position(void)
{
    return motor_config.position;
}

void update_motor_status(MOTOR_RUNNING_MODE_E motor_status)
{
    struct _knob_message *send_message;
    send_message = &MOTOR_MSG;
    send_message->ucMessageID = motor_status;
    xQueueSend(motor_msg_Queue, &send_message, (TickType_t)0);
}

void update_motor_config(int status)
{
    motor_config = super_knob_configs[status];
}

void Task_foc(void *pvParameters)
{
    (void)pvParameters;

    update_motor_status(MOTOR_INIT);
    //与I2C接线有关  设置频率
    I2Cone.begin(19, 18, 400000UL);
    //初始化传感器
    sensor.init(&I2Cone);
    //传感器与电机连接
    motor.linkSensor(&sensor);
    //设置驱动器电压
    driver.voltage_power_supply = 12;
    //驱动器初始化
    driver.init();
    //驱动器与电机连接
    motor.linkDriver(&driver);
    //传感器与电机校准参数
    motor.voltage_sensor_align = 3;
    //设置FOC调制类型
    motor.foc_modulation = FOCModulationType::SpaceVectorPWM;
    //设置控制环类型：先设置为角度控制，方便初次定位
    motor.controller = MotionControlType::angle;

    //控制环 PID控制器
    motor.PID_velocity.P = 0.08;
    motor.PID_velocity.I = 4;
    motor.PID_velocity.D = 0.0002;
    motor.LPF_velocity.Tf = 0.02; //速度低通滤波
    motor.P_angle.P = 20;         //角度P控制器
    motor.velocity_limit = 5;     //位置控制的最大速度

    #ifdef ENABLE_SIMPLEFOC_STUDIO
    //电机参数监控输出串口
    motor.useMonitoring(Serial);
    #endif
    //电机初始化
    motor.init();
    //校准编码器、启用FOC
    motor.initFOC();
    update_motor_status(MOTOR_INIT_SUCCESS);
    init_angle();
    //设置控制环类型：
    motor.controller = MotionControlType::torque;

    #ifdef ENABLE_SIMPLEFOC_STUDIO
    //定义电机ID与命令
    command.add('T', doTarget, "target angle");
    command.add('M', onMotor, (char *)"motor");
    #endif
    
    motor.loopFOC();
    update_motor_status(MOTOR_INIT_END);

    //当前相对位置
    float current_detent_center = 0;
    //上次空闲开始状态
    uint32_t last_idle_start = 0;
    //怠速检查速度
    float idle_check_velocity_ewma = 0;

    for (;;)
    {
        motor.loopFOC(); 

        // 位置/角度周期上报：限频输出单行 JSON。
        // 注意必须带上 "t":"pos" 标识，绝对不能输出裸数字行（PC 端靠纯数字行识别心跳回显）
        if (g_report_on)
        {
            const uint32_t pnow = millis();
            if (g_report_last_ms == 0 || (pnow - g_report_last_ms) >= g_report_interval_ms)
            {
                g_report_last_ms = pnow;
                g_report_active = true;
                char pos_buf[128];
                snprintf(pos_buf, sizeof(pos_buf),
                         "{\"t\":\"pos\",\"angle\":%.3f,\"position\":%d,\"velocity\":%.3f,\"haptic\":%u,\"remote\":%u}\n",
                         motor.shaft_angle, (int)motor_config.position, motor.shaft_velocity,
                         (unsigned)g_haptic_mode, (unsigned)g_remote_mode);
                Serial.print(pos_buf);
            }
        }

        //监听页面状态
        _knob_message *lvgl_message = NULL;
        if (xQueueReceive(motor_rcv_Queue, &(lvgl_message), (TickType_t)0))
        {
            Serial.print("motor_rcv_Queue --->");
            Serial.println(lvgl_message->ucMessageID);
            switch (lvgl_message->ucMessageID)
            {
            case CHECKOUT_PAGE:
            {
                //上次的相对位置
                current_detent_center = motor.shaft_angle;
                // motor.PID_velocity.limit = 10;

                const float derivative_lower_strength = motor_config.detent_strength_unit * 0.08;
                const float derivative_upper_strength = motor_config.detent_strength_unit * 0.02;
                const float derivative_position_width_lower = radians(3);
                const float derivative_position_width_upper = radians(8);
                const float raw = derivative_lower_strength + (derivative_upper_strength - derivative_lower_strength) / (derivative_position_width_upper - derivative_position_width_lower) * (motor_config.position_width_radians - derivative_position_width_lower);
                // CLAMP可以将随机变化的值限制在一个给定的区间[min,max]内
                motor.PID_velocity.D = CLAMP(
                    raw,
                    min(derivative_lower_strength, derivative_upper_strength),
                    max(derivative_lower_strength, derivative_upper_strength));

                //存在页面切换 就震动一下
                motor_shake(2, 2);

                if(motor_config.motor_auto){
                    motor.controller = MotionControlType::angle;
                    //速度PID控制器
                    motor.PID_velocity.P = 0.2;
                    motor.PID_velocity.I = 0.3;
                    motor.PID_velocity.D = 0.0000;
                    //速度低通滤波
                    motor.LPF_velocity.Tf = 0.02;
                    //角度P控制器和。
                    motor.P_angle.P = 20;
                    //位置控制的最大速度
                    motor.velocity_limit = 100;
                    motor.move(0);
                }else{
                    motor.controller = MotionControlType::torque;
                    //控制环 PID控制器
                    motor.PID_velocity.P = 0.08;
                    motor.PID_velocity.I = 4;
                    motor.PID_velocity.D = 0.0002;
                    motor.LPF_velocity.Tf = 0.02; //速度低通滤波
                    motor.P_angle.P = 20;         //角度P控制器
                    motor.velocity_limit = 5;     //位置控制的最大速度
                }

            }
            break;
            case BUTTON_CLICK:
                motor_shake(2, 2);
                break;   
            case MOTOR_AUTO_RUN:
                target_velocity = 0;
                break;
            default:
                break;
            }
        }

        // 串口远程控制优先：接管后不再走 detent 力矩算法
        if (g_remote_mode != REMOTE_OFF)
        {
            const uint32_t rnow = millis();
            float rdt = (g_remote_last_ms == 0) ? 0.001f : (rnow - g_remote_last_ms) / 1000.0f;
            g_remote_last_ms = rnow;
            if (rdt <= 0 || rdt > 0.1f) rdt = 0.001f;   // 防止长时间阻塞导致目标角跳变

            switch (g_remote_mode)
            {
            case REMOTE_HOLD:
                g_remote_target = g_remote_p1;
                break;

            case REMOTE_ROTATE:
                // 连续旋转：目标角按角速度累加，超出 ±100rad 时回绕防止精度损失
                g_remote_target += g_remote_p1 * g_remote_p2 * rdt;
                if (g_remote_target > 100.0f) g_remote_target -= 200.0f;
                if (g_remote_target < -100.0f) g_remote_target += 200.0f;
                break;

            case REMOTE_SWEEP:
            {
                // 往复摆动：三角波
                float range = g_remote_p1;        // 总幅度(弧度)
                const float speed = g_remote_p2;  // 角速度(弧度/秒)
                if (range < 0.01f) range = 0.01f;
                const float period = 2.0f * range / (speed > 0.01f ? speed : 0.01f);
                g_remote_phase += rdt;
                float t = fmodf(g_remote_phase, period) / period;   // 0~1
                float tri = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
                g_remote_target = g_remote_base + (tri - 0.5f) * range;
                break;
            }

            case REMOTE_STEP:
                // 跳到第 N 个档位（复用 detent 的角度栅格）
                g_remote_target = (float)g_remote_p3 * motor_config.position_width_radians;
                break;

            default:
                break;
            }

            // 角度环驱动：限速避免瞬间甩飞
            motor.controller = MotionControlType::angle;
            motor.P_angle.P = 10;
            motor.velocity_limit = CLAMP(fabsf(g_remote_p1) * 2.0f + 3.0f, 3.0f, 20.0f);
            motor.move(g_remote_target);
            vTaskDelay(1);
            continue;
        }

        // 转速伺服：速度闭环，优先于触觉模式（开启时独占控制权）
        if (g_vel_servo_on)
        {
            motor.controller = MotionControlType::velocity;
            // 速度 PID（比默认更跟手一点）
            motor.PID_velocity.P = VEL_SERVO_PID_P;
            motor.PID_velocity.I = VEL_SERVO_PID_I;
            motor.PID_velocity.D = VEL_SERVO_PID_D;
            motor.voltage_limit = VEL_SERVO_VOLTAGE_LIMIT; // 限驱动电压，防大电流
            // 硬安全限速：目标速度钳制在 ±max（max 自身也钳制在 [1, 20] rad/s）
            const float vmax = CLAMP(g_vel_servo_max, VEL_SERVO_MAX_FLOOR, VEL_SERVO_MAX_LIMIT);
            const float t = CLAMP(g_vel_servo_target, -vmax, vmax);
            motor.move(t);
            // 保留防飞车保护
            if (fabsf(motor.shaft_velocity) > 60.0f)
            {
                Serial.println("(motor.shaft_velocity) > 60 !!!");
                motor.move(0);
            }
            vTaskDelay(1);
            continue;
        }

        // 非档位触觉模式：接管力矩，完全跳过 detent 算法
        if (g_haptic_mode != HAPTIC_DETENT)
        {
            // 第一次进入该模式时捕获当前角度作为零点
            if (!g_haptic_center_valid)
            {
                g_haptic_center = motor.shaft_angle;
                // 进入自由飞轮时捕获当前速度作为惯性初值，之后靠惯性滑行
                if (g_haptic_mode == HAPTIC_FREE)
                {
                    g_free_spin_vel = motor.shaft_velocity;
                }
                g_haptic_center_valid = true;
            }

            // 中心偏移角度
            const float delta = motor.shaft_angle - g_haptic_center;
            const float velocity = motor.shaft_velocity;
            float torque = 0;

            // 速度太高则不增加扭矩，防止正反馈导致飞车
            if (fabsf(velocity) > 60)
            {
                Serial.println("(motor.shaft_velocity) > 60 !!!");
                motor.move(0);
                vTaskDelay(1);
                continue;
            }

            switch (g_haptic_mode)
            {
            case HAPTIC_SPRING:
                // 弹簧回中：无死区无档位，转得越远越重，松手线性回中
                torque = HAPTIC_SPRING_K * delta;
                torque = CLAMP(torque, -HAPTIC_TORQUE_LIMIT, HAPTIC_TORQUE_LIMIT);
                break;

            case HAPTIC_VISCOUS:
                // 粘滞搅拌：只跟转速有关，静止时无力矩，会停在任意位置
                torque = HAPTIC_VISCOUS_K * velocity;
                torque = CLAMP(torque, -HAPTIC_VISCOUS_LIMIT, HAPTIC_VISCOUS_LIMIT);
                break;

            case HAPTIC_ONEWAY:
                // 单向棘轮：正转几乎自由，反转强力阻止
                if (velocity > 0)
                {
                    torque = HAPTIC_ONEWAY_FREE_K * velocity;
                }
                else if (velocity < HAPTIC_ONEWAY_DEAD_VEL)
                {
                    torque = HAPTIC_ONEWAY_BLOCK_K * (-velocity);
                }
                else
                {
                    torque = 0; // 死区内不施力，避免抖动
                }
                torque = CLAMP(torque, -HAPTIC_ONEWAY_LIMIT, HAPTIC_ONEWAY_LIMIT);
                break;

            case HAPTIC_ENDSTOP:
            {
                // 离心限位：范围内轻微回中，两端硬限位墙
                float range = g_haptic_param;
                if (range <= 0) range = HAPTIC_ENDSTOP_DEFAULT_RANGE;
                const float over = fabsf(delta) - range;
                if (over > 0)
                {
                    // 越界后按越界深度给出反向大扭矩
                    torque = HAPTIC_ENDSTOP_HARD_K * over * (delta > 0 ? 1.0f : -1.0f);
                }
                else
                {
                    torque = HAPTIC_ENDSTOP_SOFT_K * delta;
                }
                torque = CLAMP(torque, -HAPTIC_TORQUE_LIMIT, HAPTIC_TORQUE_LIMIT);
                break;
            }

            case HAPTIC_FREE:
                // 自由飞轮：几乎没有阻尼/摩擦，拨一下能连续转好几圈才停。
                // 只有极小的粘性阻尼（防永动），绝不给复位力矩，保持滑行手感。
                torque = HAPTIC_FREE_K * velocity;
                torque = CLAMP(torque, -HAPTIC_FREE_LIMIT, HAPTIC_FREE_LIMIT);
                break;

            default:
                break;
            }

            motor.move(torque);
            vTaskDelay(1);
            continue;
        }

        if(motor_config.motor_auto){
            motor.move(target_velocity);
            #ifdef ENABLE_SIMPLEFOC_STUDIO
            motor.monitor();
            command.run();
            #endif
            continue;
        } 

        idle_check_velocity_ewma = motor.shaft_velocity * IDLE_VELOCITY_EWMA_ALPHA + idle_check_velocity_ewma * (1 - IDLE_VELOCITY_EWMA_ALPHA);
        if (fabsf(idle_check_velocity_ewma) > IDLE_VELOCITY_RAD_PER_SEC)
        {
            last_idle_start = 0;
        }
        else
        {
            if (last_idle_start == 0)
            {
                last_idle_start = millis();
            }
        }

        // 如果我们没有移动，并且我们接近中心(但不是完全在那里)，慢慢调整中心点以匹配当前位置
        // If we are not moving and we're close to the center (but not exactly there), slowly adjust the centerpoint to match the current position
        if (last_idle_start > 0 && millis() - last_idle_start > IDLE_CORRECTION_DELAY_MILLIS && fabsf(motor.shaft_angle - current_detent_center) < IDLE_CORRECTION_MAX_ANGLE_RAD)
        {
            // Serial.println("slowly adjust the centerpoint to match the current position......");
            current_detent_center = motor.shaft_angle * IDLE_CORRECTION_RATE_ALPHA + current_detent_center * (1 - IDLE_CORRECTION_RATE_ALPHA);
        }

        //到控制中心的角度 差值
        float angle_to_detent_center = motor.shaft_angle - current_detent_center;

        if (angle_to_detent_center > motor_config.position_width_radians * motor_config.snap_point && (motor_config.num_positions <= 0 || motor_config.position > 0))
        {
            current_detent_center += motor_config.position_width_radians;
            angle_to_detent_center -= motor_config.position_width_radians;
            motor_config.position--;
        }
        else if (angle_to_detent_center < -motor_config.position_width_radians * motor_config.snap_point && (motor_config.num_positions <= 0 || motor_config.position < motor_config.num_positions - 1))
        {
            current_detent_center -= motor_config.position_width_radians;
            angle_to_detent_center += motor_config.position_width_radians;
            motor_config.position++;
        }

        // CLAMP可以将随机变化的值限制在一个给定的区间[min,max]内
        //死区调整
        float dead_zone_adjustment = CLAMP(
            angle_to_detent_center,
            fmaxf(-motor_config.position_width_radians * DEAD_ZONE_DETENT_PERCENT, -DEAD_ZONE_RAD),
            fminf(motor_config.position_width_radians * DEAD_ZONE_DETENT_PERCENT, DEAD_ZONE_RAD));

        //出界
        bool out_of_bounds = motor_config.num_positions > 0 && ((angle_to_detent_center > 0 && motor_config.position == 0) || (angle_to_detent_center < 0 && motor_config.position == motor_config.num_positions - 1));
        motor.PID_velocity.limit = out_of_bounds ? 10 : 3;
        motor.PID_velocity.P = out_of_bounds ? motor_config.endstop_strength_unit * 4 : motor_config.detent_strength_unit * 4;

        //处理float类型的取绝对值
        if (fabsf(motor.shaft_velocity) > 60)
        {
            //如果速度太高 则不增加扭矩
            // Don't apply torque if velocity is too high (helps avoid positive feedback loop/runaway)
            Serial.println("(motor.shaft_velocity) > 60 !!!");
            motor.move(0);
        }
        else
        {
            float torque = motor.PID_velocity(-angle_to_detent_center + dead_zone_adjustment);
            motor.move(torque);
        }

        // Serial.println(motor_config.position);
        vTaskDelay(1);
    }
}