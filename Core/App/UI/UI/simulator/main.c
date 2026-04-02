
// /**
//  * @file main
//  *
//  */

// /*********************
//  *      INCLUDES
//  *********************/
// #ifndef _DEFAULT_SOURCE
//   #define _DEFAULT_SOURCE /* needed for usleep() */
// #endif

// #include <stdlib.h>
// #include <stdio.h>
// #include <time.h>
// #ifdef _MSC_VER
//   #include <Windows.h>
// #else
//   #include <unistd.h>
// #endif
// #include "lvgl/lvgl.h"

// /*********************
//  *      DEFINES
//  *********************/

// /**********************
//  *      TYPEDEFS
//  **********************/

// /**********************
//  *  STATIC PROTOTYPES
//  **********************/
// static lv_display_t * hal_init(int32_t w, int32_t h);

// /**********************
//  *  STATIC VARIABLES
//  **********************/

// /**********************
//  *      MACROS
//  **********************/

// /**********************
//  *   GLOBAL FUNCTIONS
//  **********************/

// extern void freertos_main(void);
// extern void setupUi(void);
// extern void register_fs(void);
// extern void time_timer_cb(lv_timer_t *timer);
// extern void battery_timer_cb(lv_timer_t *timer);

// /*********************
//  *      DEFINES
//  *********************/

// /**********************
//  *      TYPEDEFS
//  **********************/

// /**********************
//  *      VARIABLES
//  **********************/

// struct timespec ts = {.tv_sec = 0, .tv_nsec = 5000000}; //5毫秒

// /**********************
//  *  STATIC PROTOTYPES
//  **********************/

// /**********************
//  *   GLOBAL FUNCTIONS
//  **********************/

// int main(int argc, char **argv)
// {
//   (void)argc; /*Unused*/
//   (void)argv; /*Unused*/

//   /*Initialize LVGL*/
//   lv_init();

//   /*Initialize the HAL (display, input devices, tick) for LVGL*/
//   hal_init(240, 320);

//   register_fs();

//   #if LV_USE_OS == LV_OS_NONE

//   /* Run the default demo */
//   /* To try a different demo or example, replace this with one of: */
//   /* - lv_demo_benchmark(); */
//   /* - lv_demo_stress(); */
//   /* - lv_example_label_1(); */
//   /* - etc. */
//   // lv_demo_widgets();
//   setupUi();

//   lv_timer_t *time_timer = lv_timer_create(time_timer_cb, 1000, NULL);
//   lv_timer_t *battery_timer = lv_timer_create(battery_timer_cb, 1000, NULL);

//   while(1) {
//     /* Periodically call the lv_task handler.
//       * It could be done in a timer interrupt or an OS task too.*/
//     lv_timer_handler();
// #ifdef _MSC_VER
//     Sleep(5);
// #else
//     lv_tick_inc(5);
//     nanosleep(&ts, NULL);
// #endif
//   }

//   #elif LV_USE_OS == LV_OS_FREERTOS

//   /* Run FreeRTOS and create lvgl task */
//   freertos_main();

//   #endif

//   return 0;
// }

// /**********************
//  *   STATIC FUNCTIONS
//  **********************/

// /**
//  * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
//  * library
//  */
// static lv_display_t * hal_init(int32_t w, int32_t h)
// {

//   lv_group_set_default(lv_group_create());

//   lv_display_t * disp = lv_sdl_window_create(w, h);

//   lv_indev_t * mouse = lv_sdl_mouse_create();
//   lv_indev_set_group(mouse, lv_group_get_default());
//   lv_indev_set_display(mouse, disp);
//   lv_display_set_default(disp);

//   LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
//   lv_obj_t * cursor_obj;
//   cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
//   // LV_LOG_WARN("hal_init: lv_image_set_src");
//   // lv_image_set_src(cursor_obj, &mouse_cursor_icon);   /*Set the image source*/
//   lv_indev_set_cursor(mouse, cursor_obj);             /*Connect the image  object to the driver*/

//   lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
//   lv_indev_set_display(mousewheel, disp);
//   lv_indev_set_group(mousewheel, lv_group_get_default());

//   lv_indev_t * kb = lv_sdl_keyboard_create();
//   lv_indev_set_display(kb, disp);
//   lv_indev_set_group(kb, lv_group_get_default());

//   return disp;
// }
