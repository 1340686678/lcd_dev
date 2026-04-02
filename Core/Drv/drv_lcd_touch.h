#ifndef _DRV_LCD_TOUCH_H
#define _DRV_LCD_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

/*外部中断信号为 下降沿 需要硬件拉高*/

int drv_lcd_touch_init(void);

int drv_lcd_touch_pos_count(void);

typedef struct
{
	uint16_t x;
	uint16_t y;
	bool is_down;
}touch_pos_t;
int drv_lcd_touch_pos_data(touch_pos_t* data_list);

bool drv_lcd_touch_have_data(void);
void drv_lcd_clear_have_data_flag(void);
void drv_lcd_touch_irq(void);

#endif