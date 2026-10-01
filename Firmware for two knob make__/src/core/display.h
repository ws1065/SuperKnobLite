#pragma once
#include <Arduino.h>

/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-22 00:19:58
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-06 20:35:24
 */

typedef enum
{
    CHECKOUT_PAGE,
    BUTTON_CLICK,
    MOTOR_AUTO_RUN

} DISPLAY_STATUS_E;



void update_page_status(int page_status);
void Task_lvgl(void *pvParameters);
void set_encoder_read_ignore(bool status);

/*屏幕自检：纯色刷屏 + 面板寄存器回读诊断，用于区分硬件故障和固件问题*/
void display_self_test_start(void);
void display_self_test_stop(void);
void display_read_panel_diag(void);