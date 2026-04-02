#include "drv_spi.h"

#include "global.h"

#include "drv_sys_tick.h"

#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer")))

#define HANDLE		obj->init_param.work_param.handle.spi_handle.spi_4_wire_handle

// 引脚定义
#define SPI_CS_PORT    obj->init_param.work_param.hard_param.spi_param.cs_gpio
#define SPI_CS_PIN     obj->init_param.work_param.hard_param.spi_param.cs_pin
#define SPI_CS_GPIO    SPI_CS_PORT, SPI_CS_PIN
#define SPI_CS_H       HAL_GPIO_WritePin(SPI_CS_GPIO, GPIO_PIN_SET)
#define SPI_CS_L       HAL_GPIO_WritePin(SPI_CS_GPIO, GPIO_PIN_RESET)

#define SPI_SCK_PORT   obj->init_param.work_param.hard_param.spi_param.sck_gpio
#define SPI_SCK_PIN    obj->init_param.work_param.hard_param.spi_param.sck_pin
#define SPI_SCK_GPIO   SPI_SCK_PORT, SPI_SCK_PIN
#define SPI_SCK_H      HAL_GPIO_WritePin(SPI_SCK_GPIO, GPIO_PIN_SET)
#define SPI_SCK_L      HAL_GPIO_WritePin(SPI_SCK_GPIO, GPIO_PIN_RESET)

#define SPI_MOSI_PORT  obj->init_param.work_param.hard_param.spi_param.mosi_gpio
#define SPI_MOSI_PIN   obj->init_param.work_param.hard_param.spi_param.mosi_pin
#define SPI_MOSI_GPIO  SPI_MOSI_PORT, SPI_MOSI_PIN
#define SPI_MOSI_H     HAL_GPIO_WritePin(SPI_MOSI_GPIO, GPIO_PIN_SET)
#define SPI_MOSI_L     HAL_GPIO_WritePin(SPI_MOSI_GPIO, GPIO_PIN_RESET)
#define SPI_MOSI_READ  HAL_GPIO_ReadPin(SPI_MOSI_GPIO)

#define SPI_MISO_PORT  obj->init_param.work_param.hard_param.spi_param.miso_gpio
#define SPI_MISO_PIN   obj->init_param.work_param.hard_param.spi_param.miso_pin
#define SPI_MISO_GPIO  SPI_MISO_PORT, SPI_MISO_PIN
#define SPI_MISO_H     HAL_GPIO_WritePin(SPI_MISO_GPIO, GPIO_PIN_SET)
#define SPI_MISO_L     HAL_GPIO_WritePin(SPI_MISO_GPIO, GPIO_PIN_RESET)
#define SPI_MISO_READ  HAL_GPIO_ReadPin(SPI_MISO_GPIO)

// 模式判断
#define IS_HARD        (obj->init_param.work_param.work_mode < PORT_SPI_SOFT_3_WIRE)
#define IS_SOFT        (obj->init_param.work_param.work_mode >= PORT_SPI_SOFT_3_WIRE)

#define GET_TIME					(uint32_t)(drv_sys_tick_get() / 1000)
#define GET_TIME_MS				(uint32_t)(drv_sys_tick_get())
#define PASS_TIME_MS(X)		(uint32_t)drv_sys_tick_pass(X)

#define INIT_TRUE \
{	\
	obj->init_param.init_status.is_init = true;	\
	obj->init_param.init_status.init_suce_cnt++;	\
	obj->init_param.init_status.init_all_cnt++;	\
}

#define INIT_FAIL	\
{	\
	obj->init_param.init_status.is_init = false;	\
	obj->init_param.init_status.init_fail_cnt++;	\
	obj->init_param.init_status.init_all_cnt++;		\
}

#define DEINIT_TRUE	\
{	\
	obj->init_param.init_status.is_init = false;	\
	obj->init_param.init_status.deinit_suce_cnt++;	\
	obj->init_param.init_status.deinit_all_cnt++;		\
}

#define DEINIT_FAIL	\
{	\
	obj->init_param.init_status.is_init = false;	\
	obj->init_param.init_status.deinit_fail_cnt++;	\
	obj->init_param.init_status.deinit_all_cnt++;		\
}

#define RESET_TRUE	\
{	\
	obj->init_param.init_status.reset_suce_cnt++;	\
	obj->init_param.init_status.reset_all_cnt++;		\
}

#define RESET_FAIL	\
{	\
	obj->init_param.init_status.reset_fail_cnt++;	\
	obj->init_param.init_status.reset_all_cnt++;		\
}

#define SEND_TRUE	\
{	\
	obj->work_stats.send_suce_cnt++;						\
	obj->work_stats.send_all_cnt++;							\
	obj->work_stats.cont_fail_cnt = 0;					\
	obj->work_stats.last_send_time = GET_TIME;	\
}

#define SEND_FAIL	\
{	\
	obj->work_stats.send_fail_cnt++;						\
	obj->work_stats.send_all_cnt++;							\
	obj->work_stats.cont_fail_cnt++;					\
	obj->work_stats.last_send_time = GET_TIME;	\
}

#define SEND_FAIL_CONT_CNT obj->work_stats.cont_fail_cnt

#define RECV_TRUE	\
{	\
	obj->work_stats.recv_suce_cnt++;						\
	obj->work_stats.recv_all_cnt++;							\
	obj->work_stats.cont_fail_cnt = 0;					\
	obj->work_stats.last_recv_time = GET_TIME;	\
}

#define RECV_FAIL	\
{	\
	obj->work_stats.recv_fail_cnt++;						\
	obj->work_stats.recv_all_cnt++;							\
	obj->work_stats.cont_fail_cnt++;					\
	obj->work_stats.last_recv_time = GET_TIME;	\
}

/***************************************************************
* Name:   spi_delay()
* Input : 延时时间us
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description: 延时函数 单位us
***************************************************************/
static void spi_delay(uint32_t us)
{
	delay_us(us);
}

/***************************************************************
* Name:   get_spi_delay()
* Input : speed
* Output: void
* Return: 需要延时时间
* Author: hwl
* Revise: V1.0
* Description: 获取延时时间
***************************************************************/
static uint32_t get_spi_delay(spi_speed_t speed)
{
	switch(speed)
	{
		case SPI_SPEED_LOW:			// 低速模式，延时100us，约10kHz
			return 50;
		case SPI_SPEED_MEDIUM:	// 中速模式，延时10us，约100kHz
			return 5;
		case SPI_SPEED_HIGH:		// 高速模式，延时1us，约100KHz
			return 1;
		default:								// 默认中速模式
			return 1;
	}
}

/***************************************************************
* Name:   spi_write_read_byte()
* Input : obj 对象 data发送的数据
* Output: void
* Return: 接收数据
* Author: hwl
* Revise: V1.0
* Description: 软件SPI发送并接收一个字节，支持四种模式
***************************************************************/
static uint8_t soft_4_wire_write_read_byte(comm_port_obj_t* obj, uint8_t data)
{
	uint8_t recv_data = 0;
	uint8_t mode = obj->init_param.work_param.hard_param.spi_param.soft_param.mode;
	uint32_t delay = get_spi_delay(obj->init_param.work_param.hard_param.spi_param.soft_param.speed);
	
	for(uint8_t i = 0; i < 8; i++)
	{
		// 根据不同的SPI模式发送和接收数据
		if(mode == SPI_MODE_0)				// CPOL=0, CPHA=0
		{
			SPI_SCK_L;
			if(delay) spi_delay(delay);
			
			if(data & 0x80)
			{
				SPI_MOSI_H;
			}
			else
			{
				SPI_MOSI_L;
			}
			
			if(delay) spi_delay(delay);
			SPI_SCK_H;	// 上升沿，从机采样数据并输出数据
			if(delay) spi_delay(delay);
			
			recv_data <<= 1;
			
			if(SPI_MISO_READ)
			{
				recv_data |= 0x01;
			}
			
			data <<= 1;
		}
		else if(mode == SPI_MODE_1)	// CPOL=0, CPHA=1
		{
				SPI_SCK_L;
				if(delay) spi_delay(delay);
				
				if(data & 0x80)
				{
					SPI_MOSI_H;
				}
				else
				{
					SPI_MOSI_L;
				}
				
				if(delay) spi_delay(delay);
				SPI_SCK_H;	// 上升沿，数据输出
				if(delay) spi_delay(delay);
				SPI_SCK_L;	// 下降沿，从机采样数据并输出数据
				if(delay) spi_delay(delay);
				
				recv_data <<= 1;
				
				if(SPI_MISO_READ)
				{
					recv_data |= 0x01;
				}
				
				data <<= 1;
		}
		else if(mode == SPI_MODE_2)	// CPOL=1, CPHA=0
		{
			SPI_SCK_H;
			if(delay) spi_delay(delay);
			
			if(data & 0x80)
			{
				SPI_MOSI_H;
			}
			else
			{
				SPI_MOSI_L;
			}
			
			if(delay) spi_delay(delay);
			SPI_SCK_L;	// 下降沿，从机采样数据并输出数据
			if(delay) spi_delay(delay);
			
			recv_data <<= 1;
			
			if(SPI_MISO_READ)
			{
				recv_data |= 0x01;
			}
			
			data <<= 1;
		}
		else if(mode == SPI_MODE_3)  // CPOL=1, CPHA=1
		{
			SPI_SCK_H;
			if(delay) spi_delay(delay);
			
			if(data & 0x80)
			{
				SPI_MOSI_H;
			}
			else
			{
				SPI_MOSI_L;
			}
			
			if(delay) spi_delay(delay);
			SPI_SCK_L;	// 下降沿，数据输出
			if(delay) spi_delay(delay);
			SPI_SCK_H;	// 上升沿，从机采样数据并输出数据
			if(delay) spi_delay(delay);
			
			recv_data <<= 1;
			
			if(SPI_MISO_READ)
			{
				recv_data |= 0x01;
			}
			
			data <<= 1;
		}
	}

	return recv_data;
}

/***************************************************************
* Name:   soft_4_wire_community()
* Input : obj 对象 param 通讯参数
* Output: void
* Return: 0 成功 -1 失败
* Author: hwl
* Revise: V1.0
* Description: 通讯一次
***************************************************************/
static int soft_4_wire_community(struct comm_port_obj_t* obj, comm_msg_param_t param)
{
	if (obj == NULL || 
			param.send_len == 0 || param.send_msg == NULL)
	{
		return -1;
	}
	
	SPI_CS_L;
	spi_delay(1);

	for (size_t i = 0; i < param.send_len; i++)
	{
		uint8_t recv = soft_4_wire_write_read_byte(obj, param.send_msg[i]);
		if (param.recv_msg != NULL)
		{
			param.recv_msg[i] = recv;
		}
	}
	
	SPI_CS_H;

	return 0;
}

/***************************************************************
* Name:   soft_4_wire_community()
* Input : obj 对象 param 通讯参数
* Output: void
* Return: 0 成功 -1 失败
* Author: hwl
* Revise: V1.0
* Description: 通讯一次
***************************************************************/
static int hard_4_wire_community(struct comm_port_obj_t* obj, comm_msg_param_t param)
{
	if (obj == NULL || 
			param.send_len == 0 || param.send_msg == NULL)
	{
		return -1;
	}

	SPI_CS_L;
	spi_delay(1);
	
	HAL_StatusTypeDef rc;
	if (param.comm_work_mode == COMM_WORK_POLL)
	{
		rc = HAL_SPI_TransmitReceive(	HANDLE,
																	param.send_msg,
																	param.recv_msg,
																	param.send_len,
																	param.comm_time);
	}
	else if (param.comm_work_mode == COMM_WORK_DMA)
	{
		rc = HAL_SPI_TransmitReceive_DMA(	HANDLE,
																			param.send_msg,
																			param.recv_msg,
																			param.send_len);

		// 等待传输完成
		DRV_SYS_TICK_TYPE start_t = drv_sys_tick_get();
		while(HAL_SPI_GetState(HANDLE) != HAL_SPI_STATE_READY)
		{
			if (drv_sys_tick_pass(start_t) > param.comm_time)
			{
				rc = HAL_ERROR;
				break;
			}
		}
	}
	
	SPI_CS_H;

	return rc == HAL_OK ? 0 : -1;
}

static struct community_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_COMMUNITY_FUNC func;
}community_func_list[] = {
	[0] = {
		.mode = PORT_SPI_SOFT_4_WIRE,
		.func = soft_4_wire_community,
	},
	[1] = {
		.mode = PORT_SPI_HARD_4_WIRE,
		.func = hard_4_wire_community,
	},
};

/***************************************************************
 * Name:				soft_4_wire_init()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	SPI初始化
 ***************************************************************/
static int soft_4_wire_init(comm_port_obj_t* obj)
{
	if (obj == NULL ||
			SPI_CS_PORT == NULL ||
			SPI_SCK_PORT == NULL ||
			SPI_MISO_PORT == NULL ||
			SPI_MOSI_PORT == NULL)
	{
		return -1;
	}


	uint8_t mode = obj->init_param.work_param.hard_param.spi_param.soft_param.mode;

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/*CS引脚初始化*/
	SPI_CS_H;

	HAL_GPIO_WritePin(SPI_CS_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = SPI_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);
	
	
	/*SCK引脚初始化*/
	if (mode == SPI_MODE_0 || mode == SPI_MODE_1)
	{
		SPI_SCK_L;
	}
	else
	{
		SPI_SCK_H;
	}

	GPIO_InitStruct.Pin = SPI_SCK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_SCK_PORT, &GPIO_InitStruct);
	
	
	/*MOSI引脚初始化*/
	SPI_MOSI_H;
	
	HAL_GPIO_WritePin(SPI_MOSI_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = SPI_MOSI_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_MOSI_PORT, &GPIO_InitStruct);
	
	/*MISO引脚初始化*/
	GPIO_InitStruct.Pin = SPI_MISO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_MISO_PORT, &GPIO_InitStruct);

	return 0;
}

/***************************************************************
 * Name:				hard_4_wire_init()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	SPI初始化
 ***************************************************************/
static int hard_4_wire_init(comm_port_obj_t* obj)
{
	if (obj == NULL||
			HANDLE == NULL)
	{
		return -1;
	}
	
	if (HAL_SPI_Init(HANDLE) != HAL_OK)
	{
		delay_us(1);
		return -1;
	}
	else
	{
		delay_us(1);
		return 0;
	}
}

static struct init_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_INIT_FUNC func;
}init_func_list[] = {
	[0] = {
		.mode = PORT_SPI_SOFT_4_WIRE,
		.func = soft_4_wire_init,
	},
	[1] = {
		.mode = PORT_SPI_HARD_4_WIRE,
		.func = hard_4_wire_init,
	},
};

/***************************************************************
 * Name:				soft_4_wire_deinit()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	SPI去初始化
 ***************************************************************/
static int soft_4_wire_deinit(comm_port_obj_t* obj)
{
	if (obj == NULL ||
			SPI_CS_PORT == NULL ||
			SPI_SCK_PORT == NULL ||
			SPI_MISO_PORT == NULL ||
			SPI_MOSI_PORT == NULL)
	{
		return -1;
	}

	/*CS引脚去初始化*/
	HAL_GPIO_DeInit(SPI_CS_GPIO);
	HAL_GPIO_DeInit(SPI_SCK_GPIO);
	HAL_GPIO_DeInit(SPI_MISO_GPIO);
	HAL_GPIO_DeInit(SPI_MOSI_GPIO);
	
	return 0;
}

/***************************************************************
 * Name:				hard_4_wire_deinit()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	SPI去初始化
 ***************************************************************/
static int hard_4_wire_deinit(comm_port_obj_t* obj)
{
	if (obj == NULL||
			HANDLE == NULL)
	{
		return -1;
	}
	
	if (HAL_SPI_DeInit(HANDLE) != HAL_OK)
	{
		delay_us(1);
		return -1;
	}
	else
	{
		delay_us(1);
		return 0;
	}
}

static struct deinit_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_DEINIT_FUNC func;
}deinit_func_list[] = {
	[0] = {
		.mode = PORT_SPI_SOFT_4_WIRE,
		.func = soft_4_wire_deinit,
	},
	[1] = {
		.mode = PORT_SPI_HARD_4_WIRE,
		.func = hard_4_wire_deinit,
	},
};

/***************************************************************
 * Name:				init()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	初始化
 ***************************************************************/
static int init(comm_port_obj_t* obj)
{
	if (obj == NULL)
	{
		goto F;
	}
	
	for (size_t i = 0; i < sizeof(init_func_list) / sizeof(struct init_func_list_t); i++)
	{
		if (init_func_list[i].mode == obj->init_param.work_param.work_mode)
		{
			if (init_func_list[i].func(obj) == 0)
			{
				goto T;
			}
			else
			{
				goto F;
			}
		}
	}
	
	goto F;
	
T:
	INIT_TRUE;
	return 0;

F:
	INIT_FAIL;
	return -1;
}

/***************************************************************
 * Name:				deinit()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	取消初始化
 ***************************************************************/
static int deinit(comm_port_obj_t* obj)
{
	if (obj == NULL)
	{
		goto F;
	}
	
	for (size_t i = 0; i < sizeof(deinit_func_list) / sizeof(struct deinit_func_list_t); i++)
	{
		if (deinit_func_list[i].mode == obj->init_param.work_param.work_mode)
		{
			if (deinit_func_list[i].func(obj) == 0)
			{
				goto T;
			}
			else
			{
				goto F;
			}
		}
	}
	
	goto F;
	
T:
	DEINIT_TRUE;
	return 0;

F:
	DEINIT_FAIL;
	return -1;
}

/***************************************************************
* Name:				reset()
* Input :			obj 对象
* Output:			void
* Return:			0 成功 1失败
* Author:			hwl
* Revise:			V1.0
* Description:	重置I2C
***************************************************************/
static int reset(comm_port_obj_t* obj)
{
	if (obj == NULL)
	{
		goto F;
	}
	
	if (obj->deinit_func(obj) != 0)
	{
		goto F;
	}

	// 在CS无效状态下，主机主动输出若干周期SCLK（如8~16个），迫使从机状态机退出错误采样状态
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	SPI_CS_H;
	HAL_GPIO_WritePin(SPI_CS_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = SPI_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SPI_SCK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_SCK_PORT, &GPIO_InitStruct);

	for (size_t i = 0; i < 32; i++)
	{
		HAL_GPIO_TogglePin(SPI_SCK_GPIO);
		spi_delay(100);
	}
	
	if (obj->init_func(obj) != 0)
	{
		goto F;
	}

	goto T;

T:
	RESET_TRUE;
	return 0;

F:
	RESET_FAIL;
	return -1;
}

/***************************************************************
* Name:				set_work_mode()
* Input :			obj 对象 mode 新设置的工作模式
* Output:			void
* Return:			0 成功 1失败
* Author:			hwl
* Revise:			V1.0
* Description:	重置I2C
***************************************************************/
static int set_work_mode(comm_port_obj_t* obj, enum port_work_mode_t mode)
{
	if (obj == NULL)
	{
		return -1;
	}

	if (obj->deinit_func(obj) != 0)
	{
		return -1;
	}
	
	obj->init_param.work_param.work_mode = mode;

	if (obj->init_func(obj) != 0)
	{
		return -1;
	}

	return 0;
}

/***************************************************************
* Name:				community()
* Input :			obj 句柄 comm_msg_param_t 参数
* Output:			void
* Return:			0 成功 -1失败
* Author:			hwl
* Revise:			V1.0
* Description:	I2C发送和接收
* 							send_len = 0 不发送
* 							recv_len = 0 不接收
***************************************************************/
static int community(comm_port_obj_t* obj, comm_msg_param_t param)
{
	if (obj == NULL)
	{
		goto F;
	}
	
	for (size_t i = 0; i < sizeof(community_func_list) / sizeof(struct community_func_list_t); i++)
	{
		if (community_func_list[i].mode == obj->init_param.work_param.work_mode)
		{
			if (community_func_list[i].func(obj, param) == 0)
			{
				goto T;
			}
			else
			{
				goto F;
			}
		}
	}
	
	goto F;
	
T:
	SEND_TRUE;
	RECV_TRUE;
	return 0;

F:
	SEND_FAIL;
	RECV_FAIL;

	if (SEND_FAIL_CONT_CNT > 3)
	{
		obj->reset_func(obj);
	}
	
	return -1;
}

/***************************************************************
* Name:				print_status()
* Input :			obj 对象
* Output:			void
* Return:			void
* Author:			hwl
* Revise:			V1.0
* Description:	打印状态信息
***************************************************************/
static int print_status(comm_port_obj_t* obj)
{
	if (obj == NULL)
	{
		return -1;
	}

	APP_DEBUG("STA:%s M:%s send:%d %d %d recv:%d %d %d cont:%d\r\n",
		obj->init_param.init_status.is_init == true ? "I" : "D",
		IS_HARD == true ? "H" : "S",
		obj->work_stats.send_suce_cnt,
		obj->work_stats.send_all_cnt,
		obj->work_stats.last_send_time,
		obj->work_stats.recv_suce_cnt,
		obj->work_stats.recv_all_cnt,
		obj->work_stats.last_recv_time,
		obj->work_stats.cont_fail_cnt);

	return 0;
}

extern SPI_HandleTypeDef hspi3;

comm_port_obj_t comm_port_spi3 = {
	.init_param = {
		.init_status = {0},
		.work_param = {
			.type = PORT_SPI,
			.work_mode = PORT_SPI_HARD_4_WIRE,
			.handle = {
				.spi_handle.spi_4_wire_handle = &hspi3,
			},
			.hard_param.spi_param = {
				.cs_gpio = LCD_CS_GPIO_Port,
				.cs_pin = LCD_CS_Pin,

				.sck_gpio = SPI3_SCK_GPIO_Port,
				.sck_pin = SPI3_SCK_Pin,

				.miso_gpio = SPI3_MISO_GPIO_Port,
				.miso_pin = SPI3_MISO_Pin,

				.mosi_gpio = SPI3_MOSI_GPIO_Port,
				.mosi_pin = SPI3_MOSI_Pin,

				.soft_param = {
				  .mode = SPI_MODE_0,
					.speed = 	SPI_SPEED_MEDIUM,	
				},
			},
		},
	},

	.work_stats = {0},

	.init_func = init,
	.deinit_func = deinit,
	.reset_func = reset,
	.set_work_mode_func = set_work_mode,
	.community_func = community,
	.it_cb_func = NULL,
	.print_sta_func = print_status,
};
