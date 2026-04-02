#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include <stdio.h>
#include <time.h>

#ifdef PRJ_USE_OS
#include "cmsis_os.h"
#endif

extern uint8_t get_print_level(void);

void print(const char *fmt,...);

#define APP_INFO(...)   {print(__VA_ARGS__); }											//调试等级0时打印
#define APP_DEBUG(...)	{if(get_print_level() > 0){print(__VA_ARGS__);}}
#define APP_WARING(...)	{print("%s WARM:", __func__); print(__VA_ARGS__); }	//调试等级0时打印
#define APP_ERROR(...)	{print("%s ERROR:", __func__); print(__VA_ARGS__);}						//调试等级0时打印

typedef enum
{
	RST_NO_READ = 0,
	RST_PINRST = 1, 			//NRST 引脚复位
	RST_PWRRST = 1 << 1,	//电源复位
	RST_SFTRST = 1 << 2,	//软件复位
	RST_IWDGRST = 1 << 3,	//独立看门狗复位
	RST_WWDGRST = 1 << 4,	//窗口看门狗复位
	RST_LPWRRST = 1 << 5,	//低功耗管理复位
	RST_NO_DEF = 1 << 6,
}rst_reason_t;

rst_reason_t comn_debug_rst_reason(void);
void comn_debug_rst_reson_print(rst_reason_t reason);

#ifdef PRJ_USE_OS
void set_use_os(bool is_use_os);
bool get_use_os(void);
void os_lock(bool is_lock);
void os_sem(osMutexId_t mutex, bool is_get);	//true 获取 false 释放
#endif

void sys_soft_reset(void);

#endif
