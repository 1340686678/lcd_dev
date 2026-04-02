#include "drv_lcd_touch.h"

#include "global.h"

#include "drv_i2c.h"

#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"

#define TOUCH_ADDR					0x38

#define TOUCH_POS_COUNT			0x02

#define TOUCH_POS_1					0x03
#define TOUCH_POS_2					0x09
#define TOUCH_POS_DATA_LEN	4


#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer")))

#define BUFFER_SIZE (32)
// 定义 DMA 缓冲区
DMA_BUFFER_SECTION __attribute__((aligned(32))) 
static uint8_t tx_buffer[BUFFER_SIZE] = {0};
DMA_BUFFER_SECTION __attribute__((aligned(32)))
static uint8_t rx_buffer[BUFFER_SIZE] = {0};

/***************************************************************
 * Name:	 touch_read()
 * Input : reg:寄存器地址 len:读取长度
 * Output: val:读取到的值
 * Return: -1:失败 0:成功
 * Author: hwl
 * Revise: V1.0
 * Description: 触控SOC的寄存器读取
 ***************************************************************/
static int touch_read(const uint8_t reg, uint8_t * val, const size_t len)
{
	memset(tx_buffer, 0, BUFFER_SIZE);
	memset(rx_buffer, 0, BUFFER_SIZE);
	tx_buffer[0] = TOUCH_ADDR;
	tx_buffer[1] = reg;

	int ret = comm_port_i2c1.community_func(&comm_port_i2c1, (comm_msg_param_t){
																															.comm_work_mode = COMM_WORK_DMA,
																															.send_msg = tx_buffer,
																															.send_len = 2,
																															.recv_msg = rx_buffer,
																															.recv_len = len,
																															.comm_time = 1000,
																														});



	if (ret == 0)
	{
		memcpy(val, rx_buffer, len);
	}
	
	return ret;
}

/*SOC的ID数据*/
typedef struct 
{
	uint8_t vendor_id;	//0xA8
	uint8_t soc_id[3];	//0x9F 0xA0 0xA3
}touch_soc_id;

/***************************************************************
 * Name:	 touch_get_id()
 * Input : void
 * Output: id:读取到的ID
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 获取触控SOC的ID
 ***************************************************************/
static void touch_get_id(touch_soc_id* id)
{
	touch_read(0xA8, &(id->vendor_id), 1);

	touch_read(0xA0, &(id->soc_id[0]), 1);
	touch_read(0x9F, &(id->soc_id[1]), 1);
	touch_read(0xA3, &(id->soc_id[2]), 1);
}

/***************************************************************
 * Name:	 touch_id_check()
 * Input : void
 * Output: void
 * Return: 0:校验通过 -1:未通过
 * Author: hwl
 * Revise: V1.0
 * Description: 校验触控SOC的ID
 ***************************************************************/
static int touch_id_check()
{
	touch_soc_id id = {
		.vendor_id = 0,
		.soc_id = ""
	};

	touch_get_id(&id);

	if(	id.vendor_id != 0x11 ||
			id.soc_id[0] > 0x03 ||
			id.soc_id[1] != 0x26 ||
			id.soc_id[2] != 0x64)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/***************************************************************
 * Name:	 drv_lcd_touch_init()
 * Input : void
 * Output: void
 * Return: 0:初始成功 -1:初始失败
 * Author: hwl
 * Revise: V1.0
 * Description: 触控SOC初始化
 ***************************************************************/
int drv_lcd_touch_init(void)
{
	//校验触控SOC的ID
	if(touch_id_check() != 0)
	{
		return -1;
	}

	return 0;
}

/***************************************************************
 * Name:	 drv_lcd_touch_pos_count()
 * Input : void
 * Output: void
 * Return: 获取到的触控点个数
 * Author: hwl
 * Revise: V1.0
 * Description: 获取触控点个数
 ***************************************************************/
int drv_lcd_touch_pos_count(void)
{
	uint8_t count = 0;

	if(touch_read(TOUCH_POS_COUNT, &count, 1) != 0)	//获取异常
	{
		//清空获取到的值
		count = 0;
	}

	count = count > 2 ? count = 0 : count;

	return count;
}

/***************************************************************
 * Name:	 drv_lcd_touch_pos_count()
 * Input : void
 * Output: data_list:存储触控点的队列 必须长度为 TOUCH_POS_COUNT
 * Return: 获取到的触控点个数
 * Author: hwl
 * Revise: V1.0
 * Description: 获取触控点坐标
 ***************************************************************/
int drv_lcd_touch_pos_data(touch_pos_t* data_list)
{
	/*0=false 1=true*/
//	static uint8_t pos_is_down[TOUCH_POS_COUNT] = {0x00};
	const uint8_t data_reg[TOUCH_POS_COUNT] = {TOUCH_POS_1, TOUCH_POS_2};

	uint8_t get_count = drv_lcd_touch_pos_count();

	for(uint8_t i = 0; i < get_count; i++)
	{
		uint8_t r_buf[TOUCH_POS_DATA_LEN];
		if(touch_read(data_reg[i], r_buf, TOUCH_POS_DATA_LEN) == 0)
		{
			data_list[i].x = ((uint16_t)(r_buf[0] & 0X0F) << 8) + r_buf[1];
			data_list[i].y = ((uint16_t)(r_buf[2] & 0X0F) << 8) + r_buf[3];
			data_list[i].is_down = true;
		}
	}

	return get_count;
}

volatile bool is_have_touch = false;

/***************************************************************
 * Name:	 drv_lcd_touch_have_data()
 * Input : void
 * Output: void
 * Return: true 有数据 false 没有数据
 * Author: hwl
 * Revise: V1.0
 * Description: 是否有数据
 ***************************************************************/
bool drv_lcd_touch_have_data(void)
{
	return is_have_touch;
}

/***************************************************************
 * Name:	 drv_lcd_clear_have_data_flag()
 * Input : void
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 清空数据标志
 ***************************************************************/
void drv_lcd_clear_have_data_flag(void)
{
	is_have_touch = false;
}

/***************************************************************
 * Name:	 drv_lcd_touch_irq()
 * Input : void
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 中断回调
 ***************************************************************/
void drv_lcd_touch_irq(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(TOUCH_IRQ_Pin) != 0x00U)
	{
		is_have_touch = true;
	}
}



