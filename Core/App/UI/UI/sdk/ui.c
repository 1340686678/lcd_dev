/*
 * ui.c - Animation helpers for anyui SDK
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
#include "ui.h"

void ui_flag_modify(lv_obj_t *target, int32_t flag, int value)
{
    if (value == UI_FLAG_ACTION_TOGGLE) {
        if (lv_obj_has_flag(target, flag)) {
            lv_obj_remove_flag(target, flag);
        } else {
            lv_obj_add_flag(target, flag);
        }
    } else if (value == UI_FLAG_ACTION_ADD) {
        lv_obj_add_flag(target, flag);
    } else {
        lv_obj_remove_flag(target, flag);
    }
}

void ui_state_modify(lv_obj_t *target, int32_t state, int value)
{
    if (value == UI_STATE_ACTION_TOGGLE) {
        if (lv_obj_has_state(target, state)) {
            lv_obj_remove_state(target, state);
        } else {
            lv_obj_add_state(target, state);
        }
    } else if (value == UI_STATE_ACTION_ADD) {
        lv_obj_add_state(target, state);
    } else {
        lv_obj_remove_state(target, state);
    }
}

void anim_cb_transform_x(lv_anim_t *a, int32_t x)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_obj_set_x(usr->target, x);
}

void anim_cb_transform_y(lv_anim_t *a, int32_t y)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_obj_set_y(usr->target, y);
}

void anim_cb_set_width(lv_anim_t *a, int32_t width)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_obj_set_width(usr->target, width);
}

void anim_cb_set_height(lv_anim_t *a, int32_t height)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_obj_set_height(usr->target, height);
}

void anim_cb_set_opacity(lv_anim_t *a, int32_t opacity)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_obj_set_style_opa(usr->target, opacity, 0);
}

void anim_cb_set_image_angle(lv_anim_t *a, int32_t angle)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_img_set_angle(usr->target, angle);
}

void anim_cb_set_image_zoom(lv_anim_t *a, int32_t zoom)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    lv_img_set_zoom(usr->target, zoom);
}

int32_t anim_cb_get_x(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_obj_get_x_aligned(usr->target);
}

int32_t anim_cb_get_y(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_obj_get_y_aligned(usr->target);
}

int32_t anim_cb_get_width(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_obj_get_width(usr->target);
}

int32_t anim_cb_get_height(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_obj_get_height(usr->target);
}

int32_t anim_cb_get_opacity(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_obj_get_style_opa(usr->target, 0);
}

int32_t anim_cb_get_image_angle(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_img_get_angle(usr->target);
}

int32_t anim_cb_get_image_zoom(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    return lv_img_get_zoom(usr->target);
}

void anim_cb_free_user_data(lv_anim_t *a)
{
    anim_user_data_t *usr = (anim_user_data_t *)a->user_data;
    LV_MEM_FREE(usr);
}

void ui_scr_load_with_anim(ui_scr_load_with_anim_t *params)
{
    lv_scr_load_anim(*params->new_scr_pptr, params->anim_type, params->time, params->delay, params->auto_del);

    if (params->auto_del)
    {
        if (*params->old_scr_pptr != NULL)
        {
            ui_obj_nullify(params->old_scr_pptr);
        }
    }
}

void ui_obj_nullify(lv_obj_t **obj)
{
    *obj = NULL;
}
