/*
 * @Copyright: 
 * @file name: File name
 * @Data: Do not edit
 * @LastEditor: 
 * @LastData: 
 * @Describe: 
 */
/*
 * @Copyright: 
 * @file name: File name
 * @Data: Do not edit
 * @LastEditor: 
 * @LastData: 
 * @Describe: 
 */
#include "user_app/super_knob_user_api.h"

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

static lv_obj_t* list1;

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
}
void setup_scr_screen_setting(lv_ui *ui)
{
    ui->screen_setting = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_setting, 80, 160);

    static lv_style_t opa_style;
    lv_style_init(&opa_style);
    lv_style_set_radius(&opa_style, 10);
    lv_style_set_bg_opa(&opa_style, 0);

    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_opa(&style_text, 255);
    lv_style_set_text_color(&style_text, lv_color_make(0x1e, 0x27, 0x32));

    list1 = lv_list_create(ui->screen_setting);
    lv_obj_set_size(list1, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_row(list1, 5, 0);

    /*Add buttons to the list*/
    lv_obj_t * btn1 = lv_btn_create(list1);
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(btn1, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn1, LV_SIZE_CONTENT);
    lv_obj_t * label = lv_label_create(btn1);
    lv_label_set_text(label, "RGB");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_btn_create(list1);
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);
    lv_obj_t * labe2 = lv_label_create(btn2);
    lv_label_set_text(labe2, "BLE");
    lv_obj_center(labe2);

    lv_obj_t * btn3 = lv_btn_create(list1);
    lv_obj_add_event_cb(btn3, exit_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn3, LV_SIZE_CONTENT);
    lv_obj_t * labe3 = lv_label_create(btn3);
    lv_label_set_text(labe3, "EXIT");
    lv_obj_center(labe3);




    //刷新页面调度器
    set_super_knob_page_status(SETTING_PAGE);
}