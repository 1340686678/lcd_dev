/*
 * scr_list.h - Screen list helpers for anyui SDK
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
#ifndef __SCR_LIST_H__
#define __SCR_LIST_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

typedef lv_obj_t *(*setup_func_t)(void);

void push_scr(setup_func_t setup);
setup_func_t pop_scr(void);
setup_func_t current_scr(void);
uint32_t count_scr(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
