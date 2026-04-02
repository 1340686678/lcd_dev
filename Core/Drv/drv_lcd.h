#ifndef _DRV_LCD_H
#define _DRV_LCD_H

#include <stdint.h>

#define RED					0xF800
#define GREEN				0x07E0
#define BLUE				0x001F

//定义LCD的尺寸
#define LCD_W 240
#define LCD_H 320

int drv_lcd_init(void);
void lcd_fill(uint16_t act_x,uint16_t act_y,
							uint16_t end_x,uint16_t end_y,
							const uint16_t *color);

void lcd_fill_color(const uint16_t act_x, const uint16_t act_y,
										const uint16_t end_x, const uint16_t end_y,
										const uint16_t *color);
void lcd_refresh(void);

#endif
