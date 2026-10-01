/*
 * @Descripttion: 
 * @version: 
 * @Author: congsir
 * @Date: 2022-05-27 00:22:38
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-09-11 16:47:50
 */

#include "user_app/super_knob_user_api.h"


//图片初始化
LV_IMG_DECLARE(bilibili_img);
LV_IMG_DECLARE(google_img);
LV_IMG_DECLARE(youtube_img);
LV_IMG_DECLARE(jsq_img);
LV_IMG_DECLARE(qqmusic_img);
LV_IMG_DECLARE(wyymusic_img);

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


static void bilibili_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        update_page_status(BUTTON_CLICK);
        keyboard_write_common(0x80, 50);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}

static void youtube_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        update_page_status(BUTTON_CLICK);
        keyboard_write_common(0x80, 52);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}

static void jsq_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        update_page_status(BUTTON_CLICK);
        keyboard_write_common(0x80, 51);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}

static void wyymusic_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        update_page_status(BUTTON_CLICK);
        keyboard_write_common(0x80, 49);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}

static void google_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        update_page_status(BUTTON_CLICK);
        keyboard_write_common(0x80, 53);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
    }
}




void setup_scr_screen_one_step(lv_ui *ui)
{
    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_bg_opa(&style_btn, 0); //设置背景透明

    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_opa(&style_text, 255);
    lv_style_set_text_color(&style_text, lv_color_black());

    ui->screen_one_step = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_one_step, 80, 160);

    static lv_coord_t col_dsc[] = {32, 32, 32, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {32, 32, 25, LV_GRID_TEMPLATE_LAST};
    /*Create a container with grid*/
    lv_obj_t * cont = lv_obj_create(ui->screen_one_step);
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_size(cont, 100, 160);
    lv_obj_center(cont);


    lv_obj_t *img;
    /*Cell to 0;0 and align to to the start (left/top) horizontally and vertically too*/
    lv_obj_t * google_btn = lv_btn_create(cont);
    lv_obj_add_style(google_btn, &style_btn, LV_PART_MAIN);
    lv_obj_set_size(google_btn, 32, 32);
    lv_obj_add_event_cb(google_btn, google_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(google_btn, LV_GRID_ALIGN_START, 0, 1,
                              LV_GRID_ALIGN_START, 0, 1);
    img = lv_img_create(google_btn);
    lv_img_set_src(img, &google_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);


    lv_obj_t * bilibili_btn = lv_btn_create(cont);
    lv_obj_add_style(bilibili_btn, &style_btn, LV_PART_MAIN);
    lv_obj_set_size(bilibili_btn, 32, 32);
    lv_obj_add_event_cb(bilibili_btn, bilibili_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(bilibili_btn, LV_GRID_ALIGN_START, 1, 1,
                              LV_GRID_ALIGN_END, 0, 1);
    img = lv_img_create(bilibili_btn);
    lv_img_set_src(img, &bilibili_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * youtube_btn = lv_btn_create(cont);
    lv_obj_add_style(youtube_btn, &style_btn, LV_PART_MAIN);
    lv_obj_set_size(youtube_btn, 32, 32);
    lv_obj_add_event_cb(youtube_btn, youtube_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(youtube_btn, LV_GRID_ALIGN_STRETCH, 0, 1,
                              LV_GRID_ALIGN_STRETCH, 1, 1);
    img = lv_img_create(youtube_btn);
    lv_img_set_src(img, &youtube_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * jsq_btn = lv_btn_create(cont);
    lv_obj_add_style(jsq_btn, &style_btn, LV_PART_MAIN);
    lv_obj_set_size(jsq_btn, 32, 32);
    lv_obj_add_event_cb(jsq_btn, jsq_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(jsq_btn, LV_GRID_ALIGN_STRETCH, 1, 1,
                              LV_GRID_ALIGN_STRETCH, 1, 1);
    img = lv_img_create(jsq_btn);
    lv_img_set_src(img, &jsq_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * wyymusic_btn = lv_btn_create(cont);
    lv_obj_add_style(wyymusic_btn, &style_btn, LV_PART_MAIN);
    lv_obj_set_size(wyymusic_btn, 80, 25);
    lv_obj_add_event_cb(wyymusic_btn, wyymusic_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(wyymusic_btn, LV_GRID_ALIGN_STRETCH, 0, 2,
                              LV_GRID_ALIGN_STRETCH, 2, 1);
    img = lv_img_create(wyymusic_btn);
    lv_img_set_src(img, &wyymusic_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);



    lv_obj_t * ext_btn = lv_btn_create(ui->screen_one_step);
    lv_obj_add_style(ext_btn, &style_btn, LV_PART_MAIN);
    lv_obj_add_event_cb(ext_btn, exit_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(ext_btn, 80, 10);
    lv_obj_align(ext_btn, LV_ALIGN_BOTTOM_MID, 0, 0);


    //刷新页面调度器
    set_super_knob_page_status(ONE_STEP_PAGE);
}


