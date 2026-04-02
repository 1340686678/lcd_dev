#include "global.h"

#include "drv_uart.h"

#include "main.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef PRJ_USE_OS
#include "cmsis_os.h"
extern osMutexId_t print_mutexHandle;
#endif

/***************************************************************
* Name:   print()
* Input : p_fmt 格式打印参数
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description: 格式打印函数（需要添加头文件stdarg.h），主要用于调试使用
***************************************************************/
void print(const char *p_fmt,...)
{
	#define BUF_SIZE 128
	static char s_buf[BUF_SIZE];

	va_list ap;
	va_start(ap,p_fmt);
	memset(s_buf, 0x00, sizeof(s_buf));
	size_t len = vsnprintf(s_buf, BUF_SIZE, p_fmt, ap);
	va_end(ap);

	if (len <= BUF_SIZE)
	{
		/*发送数据*/
		comm_port_uart1.community_func(&comm_port_uart1,
																(comm_msg_param_t){
																	.comm_work_mode = COMM_WORK_DMA,
																	.send_msg = (uint8_t*)s_buf,
																	.send_len = len,
																	.recv_msg = NULL,
																	.recv_len = 0,
																	.send_recv_dis = 0,
																	.comm_time = 1000
															});
	}

	return;
}

/**
 * @brief 获取打印级别函数
 * @return uint8_t 返回打印级别，当前返回值为0
 */
uint8_t get_print_level(void)
{
	return 0;	// 返回打印级别值，当前固定返回0
}
/***************************************************************
* Name:   comn_debug_rst_reason()
* Input : void
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description: 获取上次复位原因
***************************************************************/
rst_reason_t comn_debug_rst_reason(void)
{
	static rst_reason_t s_ret_res = RST_NO_READ;
	
	if(s_ret_res != RST_NO_READ)
	{
		return s_ret_res;
	}
	else
	{
		if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))	//RST复位
		{
			s_ret_res |= RST_PINRST;
		}
		if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))	//软件复位
		{
			s_ret_res |= RST_SFTRST;
		}
//		if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))	//独立看门狗复位
//		{
//			s_ret_res |= RST_IWDGRST;
//		}
//		if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))	//看门狗复位
//		{
//			s_ret_res |= RST_WWDGRST;
//		}
//		if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))	//低功耗复位
//		{
//			s_ret_res |= RST_LPWRRST;
//		}

		__HAL_RCC_CLEAR_RESET_FLAGS();	// 清除所有标志

		return s_ret_res;
	}
}

/***************************************************************
* Name:   comn_debug_rst_reson_print()
* Input : reason 复位源
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description: 打印复位源
***************************************************************/
void comn_debug_rst_reson_print(rst_reason_t reason)
{
	uint8_t print_count = 0;
	if ((reason & RST_PWRRST) == RST_PWRRST)
	{
		print_count++;
		print("PWRRST ");
	}
	if ((reason & RST_PINRST) == RST_PINRST)
	{
		print_count++;
		print("PINRST ");
	}
	if ((reason & RST_SFTRST) == RST_SFTRST)
	{
		print_count++;
		print("SFTRST ");
	}
	if ((reason & RST_IWDGRST) == RST_IWDGRST)
	{
		print_count++;
		print("IWDGRST ");
	}
	if ((reason & RST_WWDGRST) == RST_WWDGRST)
	{
		print_count++;
		print("WWDGRST ");
	}
	if ((reason & RST_LPWRRST) == RST_LPWRRST)
	{
		print_count++;
		print("LPWRRST ");
	}

	if (print_count == 0)
	{
		if (reason == 0)
		{
			print("NO_READ ");
		}
		else
		{
			print("NO DEF %d", reason);
		}
	}
}


#ifdef PRJ_USE_OS

volatile static bool g_is_use_os = false;

/***************************************************************
 * Name:   set_use_os()
 * Input : is_use_os 是否使用OS
 * Output: void
 * Return: void
 * Author: heweilong
 * Revise: V1.0
 * Description: 设置是否使用OS
 ***************************************************************/
void set_use_os(bool is_use_os)
{
	g_is_use_os = is_use_os;
}

/***************************************************************
 * Name:   get_use_os()
 * Input : void
 * Output: void
 * Return: false 没有使用 true 使用
 * Author: heweilong
 * Revise: V1.0
 * Description: 获取是否使用OS
 ***************************************************************/
bool get_use_os(void)
{
	return g_is_use_os;
}

/***************************************************************
* Name:   os_lock()
* Input : is_lock true 加锁 false 解锁
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:os状态下加锁解锁
***************************************************************/
void os_lock(bool is_lock)
{
	if (get_use_os() == false)
	{
		return;
	}
	
	if (is_lock == true)
	{
		vTaskSuspendAll();
	}
	else
	{
		xTaskResumeAll();
	}
}

/***************************************************************
* Name:   os_sem()
* Input : mutex 信号量 is_get true 获取 false 释放
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:os状态下加锁解锁
***************************************************************/
void os_sem(osMutexId_t mutex, bool is_get)
{
	if (get_use_os() == false)
	{
		return;
	}
	
	if (is_get == true)
	{
		osMutexWait(mutex, portMAX_DELAY);
	}
	else
	{
		osMutexRelease(mutex);
	}
}
#endif

/***************************************************************
* Name:	 sys_soft_reset()
* Input : void
* Output: void
* Return: void
* Author: taozhuo
* Revise: V1.0
* Description: 系统重启
***************************************************************/
void sys_soft_reset(void)
{
	while(1)
	{
		NVIC_SystemReset();
	}
}
