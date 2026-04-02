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

#ifndef __SETUP_UI_H__
#define __SETUP_UI_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "sdk.h"
#include "animations.h"


extern lv_font_t const lv_font_SourceHanSansSC_Normal_16;


lv_obj_t * setup_screen_1(void);

// 添加按钮事件函数声明
void setup_screen_1_btn_event(void);



#ifdef __cplusplus
}
#endif
#endif
