/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-27 00:22:38
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-11 09:46:38
 */
#include "user_app/super_knob_user_api.h"
static lv_obj_t * meter;
lv_timer_t *_monitor_timer;
lv_meter_indicator_t * indic;
lv_obj_t *monitor_memory_bar;
lv_obj_t *_monitor_cpu_label;
static lv_obj_t * mbox1 ;
bool msg_box_flag = false;

void exit_pc_monitor_page(void)
{
    lv_timer_del(_monitor_timer);
    msg_box_flag = false;
    PC_MONITOR.monitor_cpu = 0;
}

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

static void show_msg_box(void)
{
    mbox1 = lv_msgbox_create(super_knob_ui.screen_cpu_monitor, "msg:", "please connect to pc!!!", NULL, false);
    lv_obj_center(mbox1);
}

void monitor_timer_timeout(lv_timer_t *timer)
{
    lv_meter_set_indicator_value(meter, indic, PC_MONITOR.monitor_cpu);
    lv_bar_set_value(monitor_memory_bar, PC_MONITOR.monitor_momoey, LV_ANIM_OFF);
    lv_label_set_text_fmt(_monitor_cpu_label,"%d%%",(int)PC_MONITOR.monitor_cpu);
    set_motor_angle(PC_MONITOR.monitor_cpu/50);

    if(PC_MONITOR.monitor_cpu == 0 && msg_box_flag == false)
    {
        show_msg_box();
        msg_box_flag = true;
    }else if(PC_MONITOR.monitor_cpu != 0){
        if(mbox1 != NULL)
        {
            lv_msgbox_close(mbox1);
            mbox1 = NULL;
        }
        
    }

}

static void event_cb(lv_event_t * e)
{
    lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
    if(dsc->part != LV_PART_INDICATOR) return;

    lv_obj_t * obj= lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    lv_area_t txt_area;
    /*If the indicator is long enough put the text inside on the right*/
    if(lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white();
    }
    /*If the indicator is still short put the text out of it on the right*/
    else {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

void setup_scr_screen_cpu_monitor(lv_ui *ui)
{
    ui->screen_cpu_monitor = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_cpu_monitor, 80, 160);

        /* create slider_prg */
    static lv_style_t style_bg;
    static lv_style_t style_indic;
    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_opa(&style_text, 255);
    lv_style_set_text_color(&style_text, lv_color_black());

    lv_style_init(&style_bg);
    lv_style_set_border_color(&style_bg, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 6); /*To make the indicator smaller*/
    lv_style_set_radius(&style_bg, 6);
    lv_style_set_anim_time(&style_bg, 1000);

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&style_indic, 3);


    meter = lv_meter_create(ui->screen_cpu_monitor);
    lv_obj_align(meter, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(meter, 100, 100);

    /*Add a scale first*/
    lv_meter_scale_t * scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 21, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    //lv_meter_set_scale_major_ticks(meter, scale, 8, 2, 15, lv_color_black(), 2);


    /*Add a blue arc to the start*/
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);

    /*Make the tick lines blue at the start of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE), false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);

    /*Add a red arc to the end*/
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);

    /*Make the tick lines red at the end of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);

    /*Add a needle line indicator*/
    indic = lv_meter_add_needle_line(meter, scale, 2, lv_palette_main(LV_PALETTE_GREY), -10);

    monitor_memory_bar = lv_bar_create(ui->screen_cpu_monitor);
    lv_obj_remove_style_all(monitor_memory_bar);  /*To have a clean start*/
    lv_obj_add_event_cb(monitor_memory_bar, event_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_add_style(monitor_memory_bar, &style_bg, 0);
    lv_obj_add_style(monitor_memory_bar, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(monitor_memory_bar, 75, 30);
    lv_obj_center(monitor_memory_bar);
    lv_bar_set_value(monitor_memory_bar, 0, LV_ANIM_OFF);
    lv_obj_add_flag(monitor_memory_bar, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(monitor_memory_bar, LV_ALIGN_BOTTOM_MID, 0, -20);

    _monitor_cpu_label = lv_label_create(ui->screen_cpu_monitor);
    lv_obj_set_width(_monitor_cpu_label, 50);
    lv_obj_set_style_text_align(_monitor_cpu_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_monitor_cpu_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text_fmt(_monitor_cpu_label,"%d%%",0);

    lv_meter_set_indicator_value(meter, indic, 66);
    _monitor_timer = lv_timer_create(monitor_timer_timeout, 500, NULL);
    lv_timer_set_repeat_count(_monitor_timer, -1);

    

    //刷新页面调度器
    set_super_knob_page_status(CPU_MONITOR_PAGE);
}