/*
 * @Copyright: 
 * @file name: File name
 * @Data: Do not edit
 * @LastEditor: 
 * @LastData: 
 * @Describe: 
 */
#include "user_app/super_knob_user_api.h"
static lv_timer_t *_motor_timer;

LV_IMG_DECLARE(kunkun_1)
LV_IMG_DECLARE(kunkun_2)
LV_IMG_DECLARE(kunkun_3)
LV_IMG_DECLARE(kunkun_4)
LV_IMG_DECLARE(kunkun_5)
LV_IMG_DECLARE(kunkun_6)
LV_IMG_DECLARE(kunkun_7)
LV_IMG_DECLARE(kunkun_8)
LV_IMG_DECLARE(kunkun_9)
LV_IMG_DECLARE(kunkun_10)
LV_IMG_DECLARE(kunkun_11)
LV_IMG_DECLARE(kunkun_12)
LV_IMG_DECLARE(kunkun_13)

static const lv_img_dsc_t* anim_imgs[13] = {
    &kunkun_1,
    &kunkun_2,
    &kunkun_3,
    &kunkun_4,
    &kunkun_5,
    &kunkun_6,
    &kunkun_7,
    &kunkun_8,
    &kunkun_9,
    &kunkun_10,
    &kunkun_11,
    &kunkun_12,
    &kunkun_13
};

void exit_kunkun_page(void)
{
    lv_timer_del(_motor_timer);
}


static void timer_timeout(lv_timer_t *timer)
{
    static bool run_flag = false;
    if(run_flag){
        set_motor_angle(1);
        run_flag = false;
    }else{
        set_motor_angle(0);
        run_flag = true;
    }
}

void setup_scr_screen_kunkun(lv_ui *ui)
{
    ui->screen_kunkun = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_kunkun, 80, 160);

    static lv_style_t opa_style;
    lv_style_init(&opa_style);
    lv_style_set_radius(&opa_style, 10);
    lv_style_set_bg_opa(&opa_style, 0);

    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_opa(&style_text, 255);
    lv_style_set_text_color(&style_text, lv_color_make(0x1e, 0x27, 0x32));

    lv_obj_t * animimg0 = lv_animimg_create(ui->screen_kunkun);
    lv_obj_center(animimg0);
    lv_animimg_set_src(animimg0, (lv_img_dsc_t**) anim_imgs, 13);
    lv_animimg_set_duration(animimg0, 1000);
    lv_animimg_set_repeat_count(animimg0, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(animimg0);

    _motor_timer = lv_timer_create(timer_timeout, 300, NULL);
    lv_timer_set_repeat_count(_motor_timer, -1);

    //刷新页面调度器
    set_super_knob_page_status(KUNKUN_PAGE);
}

