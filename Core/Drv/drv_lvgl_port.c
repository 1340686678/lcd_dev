#include "drv_lvgl_port.h"
#include "lvgl.h"

#include "global.h"

#include "main.h"

// 显示刷新回调函数
void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    // 调用现有LCD驱动刷新函数
    lcd_fill(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
    
    // 通知LVGL刷新完成
    lv_display_flush_ready(disp);
}

// 显示初始化函数
void lvgl_display_init(void) {
	while (drv_lcd_init() != 0);
	// 创建显示设备
	lv_display_t * disp = lv_display_create(LCD_W, LCD_H);
	
	// 设置刷新回调
	lv_display_set_flush_cb(disp, lvgl_flush_cb);
	
	// 设置缓冲区
	__attribute__((section(".ram_buffer"))) __attribute__((aligned(32))) 
	static uint8_t buf1[LCD_W * LCD_H * sizeof(lv_color16_t)];
	lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	
	// 设置默认渲染模式
	lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void lvgl_touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    // 假设现有驱动提供此接口
		if (drv_lcd_touch_have_data() == true)
		{
			drv_lcd_clear_have_data_flag();
			touch_pos_t data_list[2] = {0};
			drv_lcd_touch_pos_data(data_list);
			// 填充LVGL输入数据结构
			data->point.x = data_list[0].x;
			data->point.y = data_list[0].y;
			data->state = data_list[0].is_down == true ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

			APP_INFO("POS1:x:%d,y:%d\r\n",data_list[0].x,data_list[0].y);
//			APP_INFO("POS2:x:%d,y:%d\r\n",data_list[1].x,data_list[1].y);
		}
}

// 触控初始化函数
void lvgl_touch_init(void) {
		while (drv_lcd_touch_init() != 0);

    // 创建输入设备
    lv_indev_t * indev = lv_indev_create();
    
    // 设置设备类型为触控
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    
    // 设置读取回调
    lv_indev_set_read_cb(indev, lvgl_touch_read_cb);
    
    // 关联显示设备
    lv_indev_set_display(indev, lv_display_get_default());
}

lv_obj_t * global_statusbar;

uint32_t lv_tick_get_cb(void)
{
	return HAL_GetTick();
}

extern void setupUi(void);

void lvgl_init(void)
{
	lv_init();
	lv_tick_set_cb(lv_tick_get_cb);
	
	lvgl_display_init();
	lvgl_touch_init();

	setupUi();
}
	

void setup_screen_1_btn_event(void)
{
	static bool is_white = false;
	lv_obj_t * scr = lv_screen_active();
	if (is_white == true)
	{
		lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	}
	else
	{
		lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
	}
	is_white = !is_white;
	return;
}
