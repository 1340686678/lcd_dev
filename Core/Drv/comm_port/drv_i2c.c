#include "drv_i2c.h"

#include "global.h"

#include "drv_sys_tick.h"

#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer")))

#define HANDLE		obj->init_param.work_param.handle.i2c_handle

// 引脚定义
#define I2C_SCL_PORT   obj->init_param.work_param.hard_param.i2c_param.scl_gpio
#define I2C_SCL_PIN    obj->init_param.work_param.hard_param.i2c_param.scl_pin
#define I2C_SCL_GPIO   I2C_SCL_PORT, I2C_SCL_PIN
#define I2C_SCL_H      HAL_GPIO_WritePin(I2C_SCL_GPIO, GPIO_PIN_SET)
#define I2C_SCL_L      HAL_GPIO_WritePin(I2C_SCL_GPIO, GPIO_PIN_RESET)

#define I2C_SDA_PORT   obj->init_param.work_param.hard_param.i2c_param.sda_gpio
#define I2C_SDA_PIN    obj->init_param.work_param.hard_param.i2c_param.sda_pin
#define I2C_SDA_GPIO   I2C_SDA_PORT, I2C_SDA_PIN
#define I2C_SDA_H      HAL_GPIO_WritePin(I2C_SDA_GPIO, GPIO_PIN_SET)
#define I2C_SDA_L      HAL_GPIO_WritePin(I2C_SDA_GPIO, GPIO_PIN_RESET)
#define I2C_SDA_R      HAL_GPIO_ReadPin(I2C_SDA_GPIO)

// 模式判断
#define IS_HARD        (obj->init_param.work_param.work_mode == PORT_I2C_HARD)
#define IS_SOFT        (!(IS_HARD))

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
* Name:   i2c_delay()
* Input : i2c_delay 延时时间us
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description: 延时函数，单位us(us级别的时间,NOP方式，4Mhz主频)
***************************************************************/
void i2c_delay(uint32_t us)
{
	delay_us(us);
}

/***************************************************************
* Name:   i2c_start()
* Input : void
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:I2C产生起始信号
*             当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号
***************************************************************/
static void i2c_start(comm_port_obj_t* obj)
{
	I2C_SDA_H;
	I2C_SCL_H;
	i2c_delay(5);
	I2C_SDA_L;
	i2c_delay(5);
	I2C_SCL_L;
	i2c_delay(5);
}

/***************************************************************
* Name:   i2c_stop()
* Input : void
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:CPU发起I2C总线停止信号
*             当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号
***************************************************************/
static void i2c_stop(comm_port_obj_t* obj)
{
	I2C_SCL_L;
	I2C_SDA_L;
	i2c_delay(5);
	I2C_SCL_H;
	i2c_delay(5);
	I2C_SDA_H;
	i2c_delay(5);
}

/***************************************************************
* Name:   sth85_i2c_wait_ack()
* Input : void
* Output: void
* Return: true:正确应答 false:无响应
* Author: hwl
* Revise: V1.0
* Description:CPU产生一个时钟，并读取器件的ACK应答信号
*             当SCL出现一个时钟(出现一个上升沿或者第9个脉冲)，读取SDA为低电平
***************************************************************/
static uint8_t i2c_wait_ack(comm_port_obj_t* obj)
{
	uint8_t ret = false;
	I2C_SCL_L;
	i2c_delay(5);
	I2C_SDA_H;	//CPU释放SDA总线
	i2c_delay(5);
	I2C_SCL_H;	//CPU驱动SCL = 1, 此时器件会返回ACK应答
	i2c_delay(5);
	if(I2C_SDA_R == 1)//CPU读取SDA口线状态
	{
		ret = false;
	}
	else
	{
		ret = true;
	}

	I2C_SCL_L;
	i2c_delay(5);

	return ret;
}

/***************************************************************
* Name:   i2c_ack()
* Input : void
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:CPU产生一个ACK信号
*             当SCL出现一个时钟(出现一个上升沿或者第9个脉冲)，SDA应输出为低电平
***************************************************************/
static void i2c_ack(comm_port_obj_t* obj)
{
	I2C_SDA_L;//CPU驱动SDA = 0
	i2c_delay(5);
	I2C_SCL_H;//CPU产生1个时钟
	i2c_delay(5);
	I2C_SCL_L;
	i2c_delay(5);
	I2C_SDA_H;//CPU释放SDA总线
}

/***************************************************************
* Name:   i2c_no_ack()
* Input : void
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:CPU不产生一个ACK信号
*             当SCL出现一个时钟(出现一个上升沿或者第9个脉冲)，SDA应输出为高电平
***************************************************************/
static void i2c_no_ack(comm_port_obj_t* obj)
{
	I2C_SCL_L;
	i2c_delay(5);
	I2C_SDA_H;
	i2c_delay(5);
	I2C_SCL_H;
	i2c_delay(5);
	I2C_SDA_L;
	i2c_delay(5);
}

/***************************************************************
* Name:   i2c_write_byte()
* Input : ucData待发送的数据
* Output: void
* Return: void
* Author: hwl
* Revise: V1.0
* Description:I2C发送一个字节
***************************************************************/
static void i2c_write_byte(comm_port_obj_t* obj, uint8_t data)
{
	I2C_SCL_L;

	/*先发送字节的高位bit7*/
	for(uint8_t i=0; i<8; i++)
	{
		if(data & 0x80)
		{
			I2C_SDA_H;
		}
		else
		{
			I2C_SDA_L;
		}
		i2c_delay(5);

		I2C_SCL_H;
		i2c_delay(5);

		I2C_SCL_L;
		if(i == 7)
		{
			I2C_SDA_L; //释放总线
		}
		data <<= 1;//左移一个bit
		i2c_delay(5);
	}
}

/***************************************************************
* Name:   i2c_read_byte()
* Input : void
* Output: void
* Return: 读取到的一个字节
* Author: hwl
* Revise: V1.0
* Description:从I2C设备读取一字节
***************************************************************/
static uint8_t i2c_read_byte(comm_port_obj_t* obj)
{
	I2C_SDA_H;
	
	/*读到第1个bit为数据的bit7*/
	uint8_t data = 0;
	for(uint8_t i=0; i<8; i++)
	{
		data <<= 1;

		I2C_SCL_L;
		i2c_delay(5);

		I2C_SCL_H;
		i2c_delay(5);

		if(I2C_SDA_R)
		{
			data++;
		}
	}

	I2C_SCL_L;

	return data;
}


/***************************************************************
 * Name:				soft_init()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	初始化
 ***************************************************************/
static int soft_init(comm_port_obj_t* obj)
{
	if (obj == NULL ||
			I2C_SCL_PORT== NULL ||
			I2C_SDA_PORT == NULL)
	{
		return -1;
	}

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*SCL*/
	HAL_GPIO_WritePin(I2C_SCL_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = I2C_SCL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);

	/*SDA*/
	HAL_GPIO_WritePin(I2C_SDA_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

	I2C_SDA_H;
	I2C_SCL_H;

	/*给一个停止信号, 复位I2C总线上的所有设备到待机模式*/
	i2c_stop(obj);

	return 0;
}

/***************************************************************
 * Name:				hard_init()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	初始化
 ***************************************************************/
static int hard_init(comm_port_obj_t* obj)
{
	/*开启I2C功能*/
	if (HAL_I2C_Init(HANDLE) != HAL_OK)
	{
		return -1;
	}
	
	/*延时1ms*/
	i2c_delay(1000);

	return 0;
}

static struct init_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_INIT_FUNC func;
}init_func_list[] = {
	[0] = {
		.mode = PORT_I2C_SOFT,
		.func = soft_init,
	},
	[1] = {
		.mode = PORT_I2C_HARD,
		.func = hard_init,
	},
};


/***************************************************************
 * Name:				soft_deinit()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	去初始化
 ***************************************************************/
static int soft_deinit(comm_port_obj_t* obj)
{
	if (obj == NULL ||
			I2C_SCL_PORT== NULL ||
			I2C_SDA_PORT == NULL)
	{
		return -1;
	}

	/*CS引脚去初始化*/
	HAL_GPIO_DeInit(I2C_SCL_GPIO);
	HAL_GPIO_DeInit(I2C_SDA_GPIO);
	
	return 0;
}

/***************************************************************
 * Name:				hard_4_wire_deinit()
 * Input :			obj 对象
 * Output:			void
 * Return:			0 成功 -1失败
 * Author:			hwl
 * Revise:			V1.0
 * Description:	去初始化
 ***************************************************************/
static int hard_deinit(comm_port_obj_t* obj)
{
	if (obj == NULL||
			HANDLE == NULL)
	{
		return -1;
	}

	/*关闭I2C功能*/
	if (HAL_I2C_DeInit(HANDLE) != HAL_OK)
	{
		return -1;
	}

	/*延时1ms*/
	i2c_delay(1000);
	return 0;
}

static struct deinit_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_DEINIT_FUNC func;
}deinit_func_list[] = {
	[0] = {
		.mode = PORT_I2C_SOFT,
		.func = soft_deinit,
	},
	[1] = {
		.mode = PORT_I2C_HARD,
		.func = hard_deinit,
	},
};

/***************************************************************
* Name:   soft_community()
* Input : obj 对象 param 通讯参数
* Output: void
* Return: 0 成功 -1 失败
* Author: hwl
* Revise: V1.0
* Description: 通讯一次
***************************************************************/
static int soft_community(struct comm_port_obj_t* obj, comm_msg_param_t param)
{
	if (obj == NULL || 
			param.send_len == 0 || param.send_msg == NULL)
	{
		return -1;
	}
	
	uint16_t w_addr = ((uint16_t)param.send_msg[0]) << 1;
	uint16_t r_addr = w_addr + 1;

	i2c_start(obj);

	/*发送地址*/
	i2c_write_byte(obj, w_addr);
	if(i2c_wait_ack(obj) == false)
	{
		i2c_stop(obj);
		return -1;
	}

	i2c_delay(10);

	/*发送数据*/
	for (size_t i = 1; i < param.send_len; i++)
	{
		i2c_write_byte(obj, param.send_msg[i]);
		if(i2c_wait_ack(obj) == false)//等待ACK
		{
			i2c_stop(obj);
			return -1;
		}
	}

	i2c_stop(obj);
	i2c_delay(10);

	/*开始信号*/
	i2c_start(obj);

	/*发送读指令，前7位从机地址，bit0是读写标志(0-写；1-读)*/
	i2c_write_byte(obj, r_addr);
	i2c_ack(obj);//产生ACK应答

	for (size_t i = 0; i < param.recv_len; i++)
	{
		param.recv_msg[i] = i2c_read_byte(obj);
		if (i < param.recv_len - 1)
		{
			i2c_ack(obj);//产生ACK应答
		}
	}
	i2c_no_ack(obj);
	
	i2c_stop(obj);
	i2c_delay(10);

	return 0;
}

/***************************************************************
* Name:   hard_community()
* Input : obj 对象 param 通讯参数
* Output: void
* Return: 0 成功 -1 失败
* Author: hwl
* Revise: V1.0
* Description: 通讯一次
***************************************************************/
static int hard_community(struct comm_port_obj_t* obj, comm_msg_param_t param)
{
	if (obj == NULL || 
			param.send_len == 0 || param.send_msg == NULL)
	{
		return -1;
	}
	
	uint16_t w_addr = ((uint16_t)param.send_msg[0]) << 1;
	uint16_t r_addr = w_addr + 1;

	HAL_StatusTypeDef rc = HAL_OK;
	if (param.comm_work_mode == COMM_WORK_POLL)
	{
		rc += HAL_I2C_Master_Transmit(HANDLE,
																	w_addr,
																	param.send_msg + 1, 
																	param.send_len - 1,
																	param.comm_time / 2);

		rc += HAL_I2C_Master_Receive(	HANDLE,
																	r_addr,
																	param.recv_msg,
																	param.recv_len,
																	param.comm_time / 2);
	}
	else if (param.comm_work_mode == COMM_WORK_DMA)
	{
		rc = HAL_I2C_Master_Transmit_DMA(	HANDLE,
																			w_addr,
																			param.send_msg + 1,
																			param.send_len - 1);

		// 等待传输完成
		DRV_SYS_TICK_TYPE start_t = drv_sys_tick_get();
		while(HAL_I2C_GetState(HANDLE) != HAL_I2C_STATE_READY)
		{
			if (drv_sys_tick_pass(start_t) > param.comm_time / 2)
			{
				rc = HAL_ERROR;
				break;
			}
		}
		
		rc = HAL_I2C_Master_Receive_DMA(	HANDLE,
																			r_addr,
																			param.recv_msg,
																			param.recv_len);

		// 等待传输完成
		start_t = drv_sys_tick_get();
		while(HAL_I2C_GetState(HANDLE) != HAL_I2C_STATE_READY)
		{
			if (drv_sys_tick_pass(start_t) > param.comm_time / 2)
			{
				rc = HAL_ERROR;
				break;
			}
		}
	}

	return rc == HAL_OK ? 0 : -1;
}

static struct community_func_list_t{
	enum port_work_mode_t mode;
	COMM_PORT_COMMUNITY_FUNC func;
}community_func_list[] = {
	[0] = {
		.mode = PORT_I2C_SOFT,
		.func = soft_community,
	},
	[1] = {
		.mode = PORT_I2C_HARD,
		.func = hard_community,
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

	/*通讯总线重置*/
	/*IO初始化外输出模式*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*SCL*/
	HAL_GPIO_WritePin(I2C_SCL_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = I2C_SCL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);

	/*SDA*/
	HAL_GPIO_WritePin(I2C_SDA_GPIO, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

	/*生成停止条件*/
	I2C_SCL_L;
	delay_us(100000);

	I2C_SDA_L;
	delay_us(100000);

	I2C_SCL_H;
	delay_us(100000);

	/*SDA拉高 SCL翻转九次以上 重置I2C的通讯接口 */
	I2C_SDA_H;
	for(int i = 0; i < 10; i++)
	{
		HAL_GPIO_TogglePin(I2C_SDA_GPIO);
		delay_us(100000);
	}

	HAL_GPIO_DeInit(I2C_SDA_GPIO);
	HAL_GPIO_DeInit(I2C_SCL_GPIO);
	i2c_delay(100000);
	
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
* Name:				print_i2c_info()
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

extern I2C_HandleTypeDef hi2c1;
comm_port_obj_t comm_port_i2c1 = {
	.init_param = {
		.init_status = {0},
		.work_param = {
			.type = PORT_I2C,
			.work_mode = PORT_I2C_HARD,
			.handle = {
				.i2c_handle = &hi2c1,
			},
			.hard_param.i2c_param = {
				.scl_gpio = I2C1_SCL_GPIO_Port,
				.scl_pin = I2C1_SCL_Pin,

				.sda_gpio = I2C1_SDA_GPIO_Port,
				.sda_pin = I2C1_SDA_Pin,
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
