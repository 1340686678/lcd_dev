#ifndef __COMM_PORT_OBJ_H_
#define __COMM_PORT_OBJ_H_

#include "drv_lwrb.h"

#include "main.h"
#include "stm32h7xx.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct comm_port_obj_t comm_port_obj_t;

typedef struct
{
	bool is_init;

	uint32_t init_suce_cnt;
	uint32_t init_fail_cnt;
	uint32_t init_all_cnt;

	uint32_t deinit_suce_cnt;
	uint32_t deinit_fail_cnt;
	uint32_t deinit_all_cnt;

	uint32_t reset_suce_cnt;
	uint32_t reset_fail_cnt;
	uint32_t reset_all_cnt;
}init_status_t;

// 在drv_spi.h中添加SPI速度配置枚举
typedef enum {
	SPI_SPEED_LOW = 0,	// 低速模式，约100kHz
	SPI_SPEED_MEDIUM,		// 中速模式，约1MHz
	SPI_SPEED_HIGH,			// 高速模式，约5MHz
} spi_speed_t;

typedef struct
{
	enum port_type_t
	{
		PORT_I2C = 0,
		PORT_UART,
		PORT_SPI,
	}type;

	enum port_work_mode_t
	{
		PORT_I2C_HARD = 0,
		PORT_I2C_SOFT,
		PORT_UART_DMA_IDLE,
		PORT_UART_PASSTH,
		PORT_SPI_HARD_3_WIRE,	//CS SCK SDA
		PORT_SPI_HARD_4_WIRE,	//CS SCK MISO MOSI
		PORT_SPI_HARD_5_WIRE,	//CS SCK MISO MOSI_1 MOS1_2
		PORT_SPI_HARD_6_WIRE,	//CS SCK MISO MOSI_1 MOS1_2 MOS1_3 MOS1_4
		PORT_SPI_SOFT_3_WIRE,	//CS SCK SDA
		PORT_SPI_SOFT_4_WIRE,	//CS SCK MISO MOSI
	}work_mode;

	union port_hal_handle_t
	{
		I2C_HandleTypeDef* i2c_handle;
		UART_HandleTypeDef* uart_handle;	// 必须开启DMA+空闲中断
		union
		{
			SPI_HandleTypeDef* spi_4_wire_handle;
//			QSPI_HandleTypeDef* spi_6_wire_handle;
		}spi_handle;
		
	}handle;

	union port_hard_param_t		//硬件参数
	{
		struct uart_hard_param_t
		{
			GPIO_TypeDef* rx_gpio;
			uint16_t rx_pin;

			GPIO_TypeDef* tx_gpio;
			uint16_t tx_pin;
		}uart_param;

		struct i2c_hard_param_t
		{
			GPIO_TypeDef* scl_gpio;
			uint16_t scl_pin;

			GPIO_TypeDef* sda_gpio;
			uint16_t sda_pin;
		}i2c_param;

		struct spi_hard_param_t
		{
			GPIO_TypeDef* cs_gpio;	//片选
			uint16_t cs_pin;

			GPIO_TypeDef* sck_gpio;	//时钟
			uint16_t sck_pin;
			
			GPIO_TypeDef* miso_gpio;	//从机输入
			uint16_t miso_pin;
			
			GPIO_TypeDef* mosi_gpio;	//主机输出
			uint16_t mosi_pin;
			
			GPIO_TypeDef* io3_gpio;
			uint16_t io3_pin;

			GPIO_TypeDef* io4_gpio;
			uint16_t io4_pin;

			struct
			{
				enum{
					SPI_MODE_0,
					SPI_MODE_1,
					SPI_MODE_2,
					SPI_MODE_3,
				}mode;

				spi_speed_t speed;
			}soft_param;
		}spi_param;
	}hard_param;

	struct port_soft_param_t	//软件参数
	{
		/*接收 软件参数*/
		struct port_soft_rx_param_t
		{
			bool is_need_clear;									//是否需要清空缓存
			lwrb_t* rx_lwrb;
			uint8_t* rx_lwrb_buf;								//接收处理缓存
			size_t rx_lwrb_buf_size;						//接收缓存大小

			uint8_t* dma_buf;										//接收缓存
			size_t dma_buf_size;								//接收缓存大小
		}rx_param;
		
		/*发送 软件参数*/
		struct port_soft_tx_param_t
		{
			uint8_t* dma_buf;										//发送缓存
			size_t dma_buf_size;								//发送缓存大小
		}tx_param;

		struct comm_port_obj_t* pass_port;	//透传端口

	}soft_param;

}comm_port_work_param_t;

typedef struct
{
	init_status_t init_status;
	comm_port_work_param_t work_param;
}comm_port_init_param_t;

typedef struct
{
	uint32_t send_suce_cnt;
	uint32_t send_fail_cnt;
	uint32_t send_all_cnt;
	uint32_t last_send_time;

	uint32_t recv_suce_cnt;
	uint32_t recv_fail_cnt;
	uint32_t recv_all_cnt;
	uint32_t last_recv_time;

	uint32_t cont_fail_cnt;	//连续失败次数
}comm_work_status_t;

typedef struct
{
	enum{
		COMM_WORK_POLL = 0,
		COMM_WORK_DMA,
		COMM_WORK_IT,
	}comm_work_mode;

	uint8_t* send_msg;	//I2C的从机地址(不包含读写位) 在[0]
	uint16_t send_len;

	uint8_t* recv_msg;
	uint16_t recv_len;

	uint32_t send_recv_dis;	//发送和接收的间隔 串口时无效 单位ms

	uint32_t comm_time;			//通讯总时长 单位ms
}comm_msg_param_t;

typedef int (*COMM_PORT_INIT_FUNC)(struct comm_port_obj_t*);
typedef int (*COMM_PORT_DEINIT_FUNC)(struct comm_port_obj_t*);
typedef int (*COMM_PORT_RESET_FUNC)(struct comm_port_obj_t*);
typedef int (*COMM_PORT_SET_MODE_FUNC)(struct comm_port_obj_t*, enum port_work_mode_t);
typedef int (*COMM_PORT_COMMUNITY_FUNC)(struct comm_port_obj_t*, comm_msg_param_t);
typedef int (*COMM_PORT_IT_CB_FUNC)(struct comm_port_obj_t*);
typedef int (*COMM_PORT_PRINT_STA_FUNC)(struct comm_port_obj_t*);

struct comm_port_obj_t
{
	comm_port_init_param_t init_param;																			//初始化信息
	comm_work_status_t work_stats;																					//工作状态统计

	COMM_PORT_INIT_FUNC				init_func;						//初始化函数
	COMM_PORT_DEINIT_FUNC			deinit_func;					//反初始化函数
	COMM_PORT_RESET_FUNC			reset_func;						//复位函数
	COMM_PORT_SET_MODE_FUNC		set_work_mode_func;		//设置工作模式
	COMM_PORT_COMMUNITY_FUNC	community_func;				//通讯函数
	COMM_PORT_IT_CB_FUNC			it_cb_func;						//中断回调函数
	COMM_PORT_PRINT_STA_FUNC	print_sta_func;				//打印状态函数
};


#endif
