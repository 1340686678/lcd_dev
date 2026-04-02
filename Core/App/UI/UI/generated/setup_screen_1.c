/**
 *
 * This file is created and owned by anyui.
 *
 * Version: 0.40.1
 *
 * COPYRIGHT 2026 anyui Team
 * All rights reserved.
 *
 * https://anyui.tech/
 *
 * Author: anyui Team
 */

#include "setup_ui.h"


lv_obj_t * screen_1 = NULL;
lv_obj_t * screen_1_btn_WAAuUo1I = NULL;
lv_obj_t * screen_1_btn_WAAuUo1I_label = NULL;
static event_table_t screen_1_event_table = {0};
static void register_sys_events(event_table_t *table);
static void init_states(void);
static void screen_1_btn_WAAuUo1I_event_handler(lv_event_t * e);
static void register_ui_events(void);
static lv_obj_t * create_ui(void);


static void register_sys_events(event_table_t *table) {
}
static void init_states(void) {
    extern lv_obj_t * global_statusbar;
    set_current_event_table(&screen_1_event_table);
}
static void screen_1_btn_WAAuUo1I_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_PRESSED: {
        setup_screen_1_btn_event();
        break;
    }
    default: {
        break;
    }
    }
}
static void register_ui_events(void) {
    lv_obj_add_event_cb(screen_1_btn_WAAuUo1I, screen_1_btn_WAAuUo1I_event_handler, LV_EVENT_ALL, NULL);
}
static lv_obj_t * create_ui(void) {
    LV_LOG_USER("Initializing screen_1 ...");
    screen_1 = lv_obj_create(NULL);
    lv_obj_set_scrollbar_mode(screen_1, LV_SCROLLBAR_MODE_OFF);
    // Add style for screen_1 - LV_PART_MAIN | LV_STATE_DEFAULT
    lv_obj_set_style_bg_color(screen_1, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(screen_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    // Create screen_1_btn_WAAuUo1I
    screen_1_btn_WAAuUo1I = lv_btn_create(screen_1);
    lv_obj_set_x(screen_1_btn_WAAuUo1I, 102);
    lv_obj_set_y(screen_1_btn_WAAuUo1I, 143);
    lv_obj_set_width(screen_1_btn_WAAuUo1I, 100);
    lv_obj_set_height(screen_1_btn_WAAuUo1I, 50);
    screen_1_btn_WAAuUo1I_label = lv_label_create(screen_1_btn_WAAuUo1I);
    lv_obj_set_scrollbar_mode(screen_1_btn_WAAuUo1I, LV_SCROLLBAR_MODE_OFF);
    lv_label_set_text(screen_1_btn_WAAuUo1I_label, "Button");
    lv_obj_set_style_pad_all(screen_1_btn_WAAuUo1I, 0, LV_STATE_DEFAULT);
    lv_obj_align(screen_1_btn_WAAuUo1I_label, LV_ALIGN_CENTER, 0, 0);
    // Add style for screen_1_btn_WAAuUo1I - LV_PART_MAIN | LV_STATE_DEFAULT
    lv_obj_set_style_text_color(screen_1_btn_WAAuUo1I, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(screen_1_btn_WAAuUo1I, &lv_font_SourceHanSansSC_Normal_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(screen_1_btn_WAAuUo1I, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    return screen_1;
}
lv_obj_t * setup_screen_1(void) {
    if (screen_1 != NULL) {
        init_states();
        return screen_1;
    }
    create_ui();
    register_ui_events();
    register_sys_events(&screen_1_event_table);
    init_states();
    return screen_1;
}
