/*
 * ui.h - Animation helpers for anyui SDK
 *
 * This file is created and owned by anyui.
 *
 * COPYRIGHT 2025 anyui Team
 * All rights reserved.
 *
 * https://anyui.tech/
 *
 * Author: anyui Team
 */
#ifndef __UI_H__
#define __UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "mem.h"

#define UI_FLAG_ACTION_ADD 0
#define UI_FLAG_ACTION_REMOVE 1
#define UI_FLAG_ACTION_TOGGLE 2

void ui_flag_modify(lv_obj_t *target, int32_t flag, int value);

#define UI_STATE_ACTION_ADD 0
#define UI_STATE_ACTION_REMOVE 1
#define UI_STATE_ACTION_TOGGLE 2

void ui_state_modify(lv_obj_t *target, int32_t state, int value);

typedef struct {
    lv_obj_t *target;
} anim_user_data_t;

void anim_cb_transform_x(lv_anim_t *a, int32_t x);
void anim_cb_transform_y(lv_anim_t *a, int32_t y);
void anim_cb_set_width(lv_anim_t *a, int32_t width);
void anim_cb_set_height(lv_anim_t *a, int32_t height);
void anim_cb_set_opacity(lv_anim_t *a, int32_t opacity);
void anim_cb_set_image_angle(lv_anim_t *a, int32_t angle);
void anim_cb_set_image_zoom(lv_anim_t *a, int32_t zoom);

int32_t anim_cb_get_x(lv_anim_t *a);
int32_t anim_cb_get_y(lv_anim_t *a);
int32_t anim_cb_get_width(lv_anim_t *a);
int32_t anim_cb_get_height(lv_anim_t *a);
int32_t anim_cb_get_opacity(lv_anim_t *a);
int32_t anim_cb_get_image_zoom(lv_anim_t *a);
int32_t anim_cb_get_image_angle(lv_anim_t *a);

void anim_cb_free_user_data(lv_anim_t *a);

void ui_obj_nullify(lv_obj_t **obj);

typedef struct {
    lv_obj_t **new_scr_pptr;
    lv_obj_t **old_scr_pptr;
    lv_scr_load_anim_t anim_type;
    uint32_t time;
    uint32_t delay;
    bool auto_del;
} ui_scr_load_with_anim_t;

void ui_scr_load_with_anim(ui_scr_load_with_anim_t *params);

/* Check LVGL version, LVGL 9.x has `LVGL_VERSION_MAJOR` defined */
#if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR >= 9
/* LVGL 9.x */
#else
/* LVGL 8.x */
#define lv_obj_remove_flag lv_obj_clear_flag
#define lv_obj_remove_state lv_obj_clear_state
#endif

#ifdef __cplusplus
}
#endif

#endif
