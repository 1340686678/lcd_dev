/*
 * mem.h - Memory helpers for anyui SDK
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
#ifndef LV_MEM_COMPAT_H
#define LV_MEM_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* Check LVGL version, LVGL 9.x has `LVGL_VERSION_MAJOR` defined */
#if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR >= 9
    /* LVGL 9.x */
    #define LV_MEM_ALLOC(size)          lv_malloc(size)
    #define LV_MEM_REALLOC(ptr, size)   lv_realloc(ptr, size)
    #define LV_MEM_FREE(ptr)            lv_free(ptr)
#else
    /* LVGL 8.x */
    #define LV_MEM_ALLOC(size)          lv_mem_alloc(size)
    #define LV_MEM_REALLOC(ptr, size)   lv_mem_realloc(ptr, size)
    #define LV_MEM_FREE(ptr)            lv_mem_free(ptr)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_MEM_COMPAT_H */
