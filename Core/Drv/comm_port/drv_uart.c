#include "drv_uart.h"

#include "global.h"

#include "drv_sys_tick.h"

#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer")))

#define HANDLE		obj->init_param.work_param.handle.uart_handle

#define TX_PORT		obj->init_param.work_param.hard_param.uart_param.tx_gpio
#define TX_PIN		obj->init_param.work_param.hard_param.uart_param.tx_pin
#define TX_GPIO		TX_PORT, TX_PIN
#define TX_H			HAL_GPIO_WritePin(TX_GPIO, GPIO_PIN_SET)
#define TX_L			HAL_GPIO_WritePin(TX_GPIO, GPIO_PIN_RESET)

#define RX_PORT		obj->init_param.work_param.hard_param.uart_param.rx_gpio
#define RX_PIN		obj->init_param.work_param.hard_param.uart_param.rx_pin
#define RX_GPIO		RX_PORT, RX_PIN
#define RX_H			HAL_GPIO_WritePin(rx_GPIO, GPIO_PIN_SET)
#define RX_L			HAL_GPIO_WritePin(rx_GPIO, GPIO_PIN_RESET)

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

#define SEND_FAIL_CONT_CNT obj->work_stats.cont_fail_cnt

#define SEND_FAIL	\
{	\
	obj->work_stats.send_fail_cnt++;						\
	obj->work_stats.send_all_cnt++;							\
	obj->work_stats.cont_fail_cnt++;					\
	obj->work_stats.last_send_time = GET_TIME;	\
}

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
	obj->work_stats.cont_fail_cnt++;						\
	obj->work_stats.last_recv_time = GET_TIME;	\
}

/***************************************************************
 * Name:				uart_dma_start_recv()
 * Input :			void
 * Output:			void
 * Return:			HAL_OK 正常
 * Author:			hwl
 * Revise:			V1.0
 * Description:	开始接收
 ***************************************************************/
static HAL_StatusTypeDef uart_dma_start_recv(comm_port_obj_t* obj)
{
	memset(	obj->init_param.work_param.soft_param.rx_param.dma_buf,
					0x00,
					obj->init_param.work_param.soft_param.rx_param.dma_buf_size);
					
	return HAL_UART_Receive_DMA(HANDLE,
															(uint8_t*)obj->init_param.work_param.soft_param.rx_param.dma_buf,
															obj->init_param.work_param.soft_param.rx_param.dma_buf_size);
}

/***************************************************************
 * Name:				uart_dma_stop_recv()
 * Input :			void
 * Output:			void
 * Return:			HAL_OK 正常
 * Author:			hwl
 * Revise:			V1.0
 * Description:	停止接收
 ***************************************************************/
static HAL_StatusTypeDef uart_dma_stop_recv(comm_port_obj_t* obj)
{
	return HAL_UART_DMAStop(HANDLE);
}

/***************************************************************
 * Name:				uart_dma_move_data()
 * Input :			dev 句柄
 * Output:			void
 * Return:			0
 * Author:			hwl
 * Revise:			V1.0
 * Description:	将数据从DMA buffer复制到Queue
 ***************************************************************/
static size_t uart_dma_move_data(comm_port_obj_t* obj)
{
	int len = obj->init_param.work_param.soft_param.rx_param.dma_buf_size
							- __HAL_DMA_GET_COUNTER(obj->init_param.work_param.handle.uart_handle->hdmarx);

	lwrb_write(	obj->init_param.work_param.soft_param.rx_param.rx_lwrb,
							obj->init_param.work_param.soft_param.rx_param.dma_buf,
							len);
	
	return 0;
}

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
		return -1;
	}

	__disable_irq();			//删除了初始化异常

	/*初始化外设*/
	if (HAL_UART_Init(HANDLE) != HAL_OK)
	{
		__enable_irq();
		INIT_FAIL;
		return -1;
	}

	switch (obj->init_param.work_param.work_mode)
	{
		case PORT_UART_DMA_IDLE:
		case PORT_UART_PASSTH:
		{
			/* 关闭 usart 空闲中断 */
			__HAL_UART_DISABLE_IT(HANDLE, UART_IT_IDLE);

			/* 开始DMA接收数据 */
			if (uart_dma_start_recv(obj) != HAL_OK)
			{
				__enable_irq();
				INIT_FAIL;
				return -1;
			}

			/* 开启空闲中断 */
			__HAL_UART_ENABLE_IT(HANDLE, UART_IT_IDLE);

			/*环形队列初始化*/
			if (obj->init_param.work_param.soft_param.rx_param.is_need_clear == true)
			{
				if (lwrb_init(obj->init_param.work_param.soft_param.rx_param.rx_lwrb,
											obj->init_param.work_param.soft_param.rx_param.rx_lwrb_buf,
											obj->init_param.work_param.soft_param.rx_param.rx_lwrb_buf_size) == 0)
				{
					__enable_irq();
					INIT_FAIL;
					return -1;
				}
			}

			__enable_irq();

			INIT_TRUE;
			return 0;
		}
		default:
		{
			break;
		}
	}
	
	return 0;
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
		return -1;
	}

	/*初始化外设*/
	if (HAL_UART_DeInit(HANDLE) != HAL_OK)
	{
		DEINIT_FAIL;
		return -1;
	}

	/*两个初始化为输出低*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/*TX*/
	HAL_GPIO_WritePin(TX_GPIO, GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = TX_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(TX_PORT, &GPIO_InitStruct);
	/*RX*/
	HAL_GPIO_WritePin(RX_GPIO, GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = RX_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(RX_PORT, &GPIO_InitStruct);


	lwrb_free(obj->init_param.work_param.soft_param.rx_param.rx_lwrb);

	DEINIT_TRUE;
	return 0;
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
		return -1;
	}

	obj->init_param.init_status.is_init = false;

	if (obj->deinit_func(obj) != 0)
	{
		RESET_FAIL;
		return -1;
	}

	if (obj->init_func(obj) != 0)
	{
		RESET_FAIL;
		return -1;
	}

	RESET_TRUE;
	return 0;
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

	obj->init_param.work_param.work_mode = mode;
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
	if (obj == NULL ||
			(param.send_len != 0 && param.send_msg == NULL) ||
			(param.recv_len != 0 && param.recv_msg == NULL) ||
			(param.send_len > obj->init_param.work_param.soft_param.tx_param.dma_buf_size))
	{
		goto F;
	}

	
	if (param.comm_work_mode == COMM_WORK_DMA)					//DMA
	{
		memcpy(obj->init_param.work_param.soft_param.tx_param.dma_buf,
														param.send_msg,
														param.send_len);
		HAL_StatusTypeDef rc = HAL_UART_Transmit_DMA(
														HANDLE,
														obj->init_param.work_param.soft_param.tx_param.dma_buf,
														param.send_len);

		if(rc == HAL_OK)
		{
			uint32_t start_t = GET_TIME_MS;
			while(PASS_TIME_MS(start_t) < param.comm_time)
			{
				if ((HAL_UART_GetState(HANDLE) & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX)
				{
					goto T;
				}
			}
		}

		goto F;
	}
	else if (param.comm_work_mode == COMM_WORK_POLL)		//POLL
	{
		if (HAL_UART_Transmit(HANDLE,
													param.send_msg,
													param.send_len,
													param.comm_time) == HAL_OK)
		{
			goto T;
		}
		else
		{
			goto F;
		}
	}
	else if (param.comm_work_mode == COMM_WORK_IT)					//DMA
	{
		if (HAL_UART_Transmit_IT(HANDLE,
													param.send_msg,
													param.send_len) == HAL_OK)
		{
			goto T;
		}
		else
		{
			goto F;
		}
	}
	
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
* Input :			dev 句柄
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

	APP_DEBUG("STA:%s send:%d %d %d recv:%d %d %d cont:%d\r\n",
		obj->init_param.init_status.is_init == true ? "I" : "D",
		obj->work_stats.send_suce_cnt,
		obj->work_stats.send_all_cnt,
		obj->work_stats.last_send_time,
		obj->work_stats.recv_suce_cnt,
		obj->work_stats.recv_all_cnt,
		obj->work_stats.last_recv_time,
		obj->work_stats.cont_fail_cnt);

	return 0;
}

/***************************************************************
 * Name:				drv_uart_it_cb()
 * Input :			obj 对象
 * Output:			void
 * Return:			void
 * Author:			hwl
 * Revise:			V1.0
 * Description:	中断回调 不能使用HAL_DELAY或其他延时
 ***************************************************************/
int it_cb(comm_port_obj_t* obj)
{
	if (obj == NULL)
	{
		return -1;
	}

	/* 判断是否产生IDLE中断 */
	if (__HAL_UART_GET_FLAG(HANDLE, UART_FLAG_IDLE) == SET)
	{
		/* 清除中断标志位　*/
		__HAL_UART_CLEAR_IDLEFLAG(HANDLE);

		/* 先关闭DMA接收 */
		uart_dma_stop_recv(obj);

		if (obj->init_param.work_param.work_mode == PORT_UART_DMA_IDLE)	//接收模式
		{
			uart_dma_move_data(obj);	//将接收数据放到队列
		}
		else if (obj->init_param.work_param.work_mode == PORT_UART_PASSTH)	//透传
		{
			obj->init_param.work_param.soft_param.pass_port->community_func(
				obj->init_param.work_param.soft_param.pass_port,
				(comm_msg_param_t){
					.comm_work_mode = COMM_WORK_POLL,
					.send_msg = obj->init_param.work_param.soft_param.rx_param.dma_buf,
					.send_len = obj->init_param.work_param.soft_param.rx_param.dma_buf_size
							- __HAL_DMA_GET_COUNTER(obj->init_param.work_param.handle.uart_handle->hdmarx),
					.recv_msg = NULL,
					.recv_len = 0,
					.send_recv_dis = 0,
					.comm_time = 1000,
				}
			);
		}
		
		/* 重新开启DMA接收 */
		uart_dma_start_recv(obj);

		RECV_TRUE;
	}

	return 0;
}

extern UART_HandleTypeDef huart1;
#define UART1_RX_BUF_SIZE 256
static lwrb_t uart1_rx_lwrb;
static uint8_t uart1_rx_fifo_buffer[UART1_RX_BUF_SIZE] = {0};
DMA_BUFFER_SECTION __attribute__((aligned(32)))
static uint8_t uart1_dma_rx_buf[UART1_RX_BUF_SIZE] = {0};	//DMA空间
DMA_BUFFER_SECTION __attribute__((aligned(32)))
static uint8_t uart1_dma_tx_buf[UART1_RX_BUF_SIZE] = {0};	//DMA空间

comm_port_obj_t comm_port_uart1 = {
	.init_param = {
		.init_status = {0},
		.work_param = {
			.type = PORT_UART,
			.work_mode = PORT_UART_DMA_IDLE,
			.handle.uart_handle = &huart1,
			.hard_param.uart_param = {
				.rx_gpio = USART1_RX_GPIO_Port,
				.rx_pin = USART1_RX_Pin,
				
				.tx_gpio = USART1_TX_GPIO_Port,
				.tx_pin = USART1_TX_Pin,
			},
			.soft_param = {
				.rx_param = {
					.is_need_clear = true,
					.rx_lwrb = &uart1_rx_lwrb,
					.rx_lwrb_buf = uart1_rx_fifo_buffer,
					.rx_lwrb_buf_size = UART1_RX_BUF_SIZE,

					.dma_buf = uart1_dma_rx_buf,
					.dma_buf_size = UART1_RX_BUF_SIZE,
				},
				.tx_param = {
					.dma_buf = uart1_dma_tx_buf,
					.dma_buf_size = UART1_RX_BUF_SIZE,
				},
				.pass_port = NULL,
			}
		},
	},

	.work_stats = {0},

	.init_func = init,
	.deinit_func = deinit,
	.reset_func = reset,
	.set_work_mode_func = set_work_mode,
	.community_func = community,
	.it_cb_func = it_cb,
	.print_sta_func = print_status,
};
