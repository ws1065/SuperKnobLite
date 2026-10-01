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