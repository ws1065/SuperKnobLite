/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-27 00:22:38
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-11 08:57:03
 */
#include "user_app/super_knob_user_api.h"
static lv_obj_t * chart;
lv_chart_series_t * ser;
int ecg_cnt = 0;
lv_coord_t ecg_sample[80] = {0};
static lv_timer_t *_motor_timer;

static void exit_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        setup_scr_screen_iot_main(&super_knob_ui);
        lv_scr_load_anim(super_knob_ui.screen_iot_main_boday, LV_SCR_LOAD_ANIM_FADE_ON, 100, 100, true);
        set_super_knob_page_status(SUPER_PAGE_BUSY);
        update_motor_config(CONFIG_RATCHET);
        update_page_status(CHECKOUT_PAGE);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}

void exit_angle_page(void)
{
    lv_timer_del(_motor_timer);
}


static void timer_timeout(lv_timer_t *timer)
{
    int ecg_tmp = ecg_sample[ecg_cnt];

    ecg_sample[ecg_cnt] = get_motor_angle()*10;
    ecg_cnt ++;
    //ecg_sample[ecg_cnt-1] = ecg_tmp;

    if(ecg_cnt == 80)
    {
        ecg_cnt = 0;
    }

    lv_chart_set_point_count(chart, 80);
    lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)ecg_sample);
}

void setup_scr_screen_motor_angle(lv_ui *ui)
{
    ui->screen_motor_angle = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_motor_angle, 80, 160);

    chart = lv_chart_create(ui->screen_motor_angle);
    lv_obj_set_size(chart, 80, 160);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -20, 20);

    /*Do not display points on the data*/
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_set_point_count(chart, 80);
    lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)ecg_sample);

    static uint32_t user_data = 10;
    _motor_timer = lv_timer_create(timer_timeout, 20, &user_data);
    lv_timer_set_repeat_count(_motor_timer, -1);

    //刷新页面调度器
    set_super_knob_page_status(MOTOR_ANGLE_PAGE);
}