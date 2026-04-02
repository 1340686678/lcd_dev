#ifndef __DRV_LVGL_PORT_H__
#define __DRV_LVGL_PORT_H__

#include "drv_lcd.h"
#include "drv_lcd_touch.h"

#include <stdint.h>
#include <stdbool.h>

void lvgl_display_init(void);
void lvgl_touch_init(void);


void lvgl_init(void);

#endif
