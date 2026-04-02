#ifndef __DRV_SYS_TICK_H__
#define __DRV_SYS_TICK_H__
#include <stdint.h>
#include <stdbool.h>

#include "global.h"

/* HAL的tick会在连续运行49天后清空 所以重写了tick 运行几万年也不清空 */

/* 时刻数据类型最大值ms */
#define DRV_SYS_TICK_MAX 0xFFFFFFFFFFFFFFFF
/* 时刻数据类型ms */
#define DRV_SYS_TICK_TYPE uint64_t
/* 需要放到 调用HAL_IncTick()的同一地方 */
void drv_sys_tick_inc(void);
/*和 HAL_GetTick() 一个用法 */
DRV_SYS_TICK_TYPE drv_sys_tick_get(void);
/* 获取距离t时刻过去多久ms */
DRV_SYS_TICK_TYPE drv_sys_tick_pass(DRV_SYS_TICK_TYPE t);

/* 延时 */
void drv_sys_delay(bool is_rtos, uint32_t ms);

#define NO_OS_DELAY(x)	drv_sys_delay(false, (x))
#define OS_DELAY(x)			drv_sys_delay(true, (x))
#define DELAY_AUTO(x)		drv_sys_delay(get_use_os(), (x))

void delay_us(uint32_t t);

#endif
