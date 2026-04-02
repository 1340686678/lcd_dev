
#include "drv_sys_tick.h"
#include "stm32h7xx.h"

#include "main.h"

#include <stdbool.h>

static DRV_SYS_TICK_TYPE g_sys_tick = 0;

extern HAL_TickFreqTypeDef uwTickFreq;

/***************************************************************
 * Name:   drv_sys_tick_inc()
 * Input : void
 * Output: void
 * Return: 运行时长
 * Author: heweilong
 * Revise: V1.0
 * Description: 系统运行时长累加
 ***************************************************************/
void drv_sys_tick_inc(void)
{
	g_sys_tick += uwTickFreq;
}

/***************************************************************
 * Name:   drv_sys_tick_get()
 * Input : void
 * Output: void
 * Return: 运行时长
 * Author: heweilong
 * Revise: V1.0
 * Description: 获取系统运行时长
 ***************************************************************/
DRV_SYS_TICK_TYPE drv_sys_tick_get(void)
{
	return g_sys_tick;
}

/***************************************************************
 * Name:		drv_sys_tick_pass()
 * Input :	t开始时刻
 * Output:	void
 * Return:	运行时长
 * Author:	heweilong
 * Revise:	V1.0
 * Description:	获取距离t时刻过去多久ms
 ***************************************************************/
DRV_SYS_TICK_TYPE drv_sys_tick_pass(DRV_SYS_TICK_TYPE t)
{
	if(drv_sys_tick_get() < t)
	{
		return (DRV_SYS_TICK_MAX - drv_sys_tick_get() + t);
	}
	else
	{
		return (drv_sys_tick_get() - t);
	}
	
}

extern __IO uint32_t uwTick;
/***************************************************************
 * Name:   HAL_IncTick()
 * Input : void
 * Output: void
 * Return: void
 * Author: heweilong
 * Revise: V1.0
 * Description: 重写HAL_IncTick
 ***************************************************************/
void HAL_IncTick(void)
{
	uwTick += uwTickFreq;
	drv_sys_tick_inc();
}

/***************************************************************
 * Name:   drv_sys_delay()
 * Input : is_rtos 是否使用RTOS ms 延时时间
 * Output: void
 * Return: void
 * Author: heweilong
 * Revise: V1.0
 * Description: 延时
 ***************************************************************/
void drv_sys_delay(bool is_rtos, uint32_t ms)
{
	if (is_rtos == true)
	{
#ifdef PRJ_USE_OS
		osDelay(ms);
#else
		HAL_Delay(ms);
#endif
	}
	else
	{
		HAL_Delay(ms);
	}
}

/***************************************************************
* Name:   Delay_us()
* Input : ustime:延时时间us
* Output: void
* Return: void
* Author: taozhuo
* Revise: V1.0
* Description: 延时函数，单位us(us级别的时间,NOP方式，170Mhz主频)
***************************************************************/
void delay_us(uint32_t t)
{
	// 根据实际系统时钟频率调整延时
	// 假设系统时钟为64MHz，每个NOP指令约为15.6ns
	// 1us = 64个NOP指令
	uint64_t count = t * (SystemCoreClock / 40000000); // 调整系数以获得精确延时
	
	while(count--)
	{
		__NOP();
	}
}
