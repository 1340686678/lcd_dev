#include "drv_lcd.h"

#include "global.h"

#include "drv_spi.h"

#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer")))
#define RAM_BUFFER_SECTION __attribute__((section(".ram_buffer")))

#define DIS_LINE 107
#define BUFFER_SIZE (LCD_W * 2 * DIS_LINE)
// 定义 DMA 缓冲区
DMA_BUFFER_SECTION __attribute__((aligned(32))) 
static uint8_t spi_tx_buffer[BUFFER_SIZE] = {0};
DMA_BUFFER_SECTION __attribute__((aligned(32)))
static uint8_t spi_rx_buffer[BUFFER_SIZE] = {0};

RAM_BUFFER_SECTION __attribute__((aligned(32)))
static uint8_t g_lcd_dis_buf[LCD_W * LCD_H * 2] = {0};

/***************************************************************
 * Name:	 spi_transfer()
 * Input : g_spi_fd:SPI设备描述符 tx:发送数据 rx:接收数据 len:发送数据长度(单位，字节)
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: spi参数数据
 * 如果 只读不发送 tx设为0x00
 ***************************************************************/
static void spi_transfer(uint8_t *tx, uint8_t *rx, const size_t len)
{
	memset(spi_tx_buffer, 0x00, sizeof(spi_tx_buffer));
	memset(spi_rx_buffer, 0x00, sizeof(spi_rx_buffer));
	memcpy(spi_tx_buffer, tx, len);

	comm_port_spi3.community_func(&comm_port_spi3, (comm_msg_param_t){
		.comm_work_mode = COMM_WORK_DMA,
		.send_msg = spi_tx_buffer,
		.send_len = len,
		.recv_msg = spi_rx_buffer,
		.recv_len = len,
		.comm_time = 1000,
	});

	memcpy(rx, spi_rx_buffer, len);
}

/***************************************************************
 * Name:	 spi_w_data()
 * Input : data:写的数据
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: spi发送
 ***************************************************************/
static void spi_w_data(const uint8_t data)
{
	uint8_t tx_buffer = data;
	uint8_t rx_buffer = 0;

	spi_transfer(&tx_buffer, &rx_buffer, 1);
}

/***************************************************************
 * Name:	 spi_r_data()
 * Input : void
 * Output: void
 * Return: 接收到的数据
 * Author: hwl
 * Revise: V1.0
 * Description: spi接收
 ***************************************************************/
static uint8_t spi_r_data(void)
{
	uint8_t tx_buffer = 0;
	uint8_t rx_buffer = 0;

	spi_transfer(&tx_buffer, &rx_buffer, 1);

	return rx_buffer;
} 

/* SET 未选中 CLR 选中 */
#define LCD_CS_SET HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_CS_CLR HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)

/* SET 命令 CLR 数据 */
#define LCD_RS_SET HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET)
#define LCD_RS_CLR HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET)

/* SET 运行 CLR 复位 */
#define LCD_RST_SET HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)
#define LCD_RST_CLR HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)

#define delay_ms(x) HAL_Delay(x)

//LCD重要参数集
typedef struct  
{
	uint16_t width;		//LCD 宽度
	uint16_t height;	//LCD 高度
	uint8_t dir;			//横屏还是竖屏控制：0，竖屏；1，横屏。
	uint16_t wramcmd;	//开始写gram指令
	uint16_t rramcmd;	//开始读gram指令
	uint16_t setxcmd;	//设置x坐标指令
	uint16_t setycmd;	//设置y坐标指令
}_lcd_dev;

static _lcd_dev g_lcd_param;


/***************************************************************
 * Name:	 lcd_w_cmd()
 * Input : cmd:写的指令
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 向lcd写指令
 ***************************************************************/
static void lcd_w_cmd(const uint8_t cmd)
{
	LCD_RS_CLR;
	LCD_CS_CLR;
	spi_w_data(cmd);
	LCD_CS_SET;
}

/***************************************************************
 * Name:	 lcd_w_data()
 * Input : data:写的数据
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 向lcd写数据
 ***************************************************************/
static void lcd_w_data(const uint8_t data)
{
	LCD_RS_SET;
	LCD_CS_CLR;
	spi_w_data(data);
	LCD_CS_SET;
}

/***************************************************************
 * Name:	 lcd_r_data()
 * Input : void
 * Output: void
 * Return: 接收到的数据
 * Author: hwl
 * Revise: V1.0
 * Description: 向lcd读数据
 ***************************************************************/
static uint8_t lcd_r_data(void)
{
	uint8_t data;
	LCD_RS_SET;
	LCD_CS_CLR;
	data = spi_r_data();
	LCD_CS_SET;
	return data;
}

/***************************************************************
 * Name:	 lcd_w_reg()
 * Input : reg:寄存器 data:写入的数据
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 写lcd寄存器数据
 ***************************************************************/
void lcd_w_reg(const uint8_t reg, const uint16_t data)
{	
	lcd_w_cmd(reg);
	lcd_w_data(data);
}

/***************************************************************
 * Name:	 lcd_r_reg()
 * Input : reg:寄存器
 * Output: void
 * Return: 寄存器数据
 * Author: hwl
 * Revise: V1.0
 * Description: 读lcd寄存器数据
 ***************************************************************/
uint8_t lcd_r_reg(const uint8_t reg)
{
	lcd_w_cmd(reg);
  return lcd_r_data();
}

/***************************************************************
 * Name:	 lcd_w_data_16bit()
 * Input : Data:需要写入的数据
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 写入16bit数据
 ***************************************************************/
void lcd_w_data_16bit(const uint16_t Data)
{
	LCD_RS_SET;
	LCD_CS_CLR;
	spi_w_data(Data>>8);
	spi_w_data(Data);
	LCD_CS_SET;
}

/***************************************************************
 * Name:	 lcd_set_windows()
 * Input : xStar:窗口开始X点 yStar:窗口开始Y点 xEnd:窗口结束Y点 yEnd:窗口结束Y点 
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 设置操作窗口
 ***************************************************************/
void lcd_set_windows(const uint16_t x_star, const uint16_t y_star, const uint16_t x_end, const uint16_t y_end)
{	
	lcd_w_cmd(g_lcd_param.setxcmd);
	lcd_w_data(x_star >> 8);
	lcd_w_data(0x00FF & x_star);
	lcd_w_data(x_end >> 8);
	lcd_w_data(0x00FF & x_end);

	lcd_w_cmd(g_lcd_param.setycmd);
	lcd_w_data(y_star >> 8);
	lcd_w_data(0x00FF & y_star);
	lcd_w_data(y_end >> 8);
	lcd_w_data(0x00FF & y_end);

	lcd_w_cmd(g_lcd_param.wramcmd);	//开始写入GRAM
}

/***************************************************************
 * Name:	 lcd_read_id()
 * Input : void
 * Output: void
 * Return: ID
 * Author: hwl
 * Revise: V1.0
 * Description: 获取屏幕ID
 ***************************************************************/
uint16_t lcd_read_id(void)
{
	uint8_t i,val[3] = {0};
	uint16_t id;

	for(i=1;i<4;i++)
	{
		lcd_w_cmd(0xD9);

		lcd_w_data(0x10+i);

		lcd_w_cmd(0xD3);

		val[i-1] = lcd_r_data();
	}

	id=val[1];
	id<<=8;
	id|=val[2];
	return id;
}

/***************************************************************
 * Name:	 lcd_refresh()
 * Input : void
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 刷新屏幕
 ***************************************************************/
void lcd_refresh(void)
{
	lcd_set_windows(0, 0, LCD_W - 1, LCD_H - 1);

	LCD_CS_CLR;
	LCD_RS_SET;
	uint16_t dis_line = 0;
	for(uint32_t i = 0; i < LCD_H;)
	{
		if ((i + DIS_LINE) < LCD_H)
		{
			dis_line = DIS_LINE;
		}
		else
		{
			dis_line = (LCD_H - i);
		}

		memset(spi_tx_buffer, 0x00, sizeof(spi_tx_buffer));
		memset(spi_rx_buffer, 0x00, sizeof(spi_rx_buffer));
		memcpy(spi_tx_buffer, g_lcd_dis_buf + i * LCD_W * 2, LCD_W * 2 * dis_line);
		
		comm_port_spi3.community_func(&comm_port_spi3, (comm_msg_param_t){
			.comm_work_mode = COMM_WORK_DMA,
			.send_msg = spi_tx_buffer,
			.send_len = LCD_W * 2 * dis_line,
			.recv_msg = spi_rx_buffer,
			.recv_len = LCD_W * 2 * dis_line,
			.comm_time = 1000,
		});


		i += dis_line;
	}

	LCD_CS_SET;
}

/***************************************************************
 * Name:	 lcd_fill()
 * Input : act_x act_y end_x end_y:要填充区域的坐标 color:填充的颜色缓存区
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 区域填充
 ***************************************************************/
void lcd_fill(const uint16_t act_x, const uint16_t act_y,
							const uint16_t end_x, const uint16_t end_y,
							const uint16_t *color)
{
	uint32_t color_num = 0;

	for(int i = act_y; i <= end_y; i++)
	{
		for(int j = act_x; j <= end_x; j++)
		{
			//更新这个点的显存
			g_lcd_dis_buf[j * 2 + i * LCD_W * 2] = (uint8_t)(color[color_num] >> 8);
			g_lcd_dis_buf[j * 2 + i * LCD_W * 2 + 1] = (uint8_t)(color[color_num]);
			color_num++;
		}
	}
	lcd_refresh();
}

/***************************************************************
 * Name:	 lcd_fill()
 * Input : act_x act_y end_x end_y:要填充区域的坐标 color:填充的颜色缓存区
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 区域填充 同一种颜色
 ***************************************************************/
void lcd_fill_color(const uint16_t act_x, const uint16_t act_y,
										const uint16_t end_x, const uint16_t end_y,
										const uint16_t *color)
{
	// uint32_t color_num = 0;

	for(int i = act_y; i <= end_y; i++)
	{
		for(int j = act_x; j <= end_x; j++)
		{
			//更新这个点的显存
			g_lcd_dis_buf[j * 2 + i * LCD_W * 2] = (uint8_t)(*color >> 8);
			g_lcd_dis_buf[j * 2 + i * LCD_W * 2 + 1] = (uint8_t)(*color);
		}
	}
}

/***************************************************************
 * Name:	 lcd_reset()
 * Input : void
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 复位LCD
 ***************************************************************/
static void lcd_reset(void)
{
	LCD_RST_SET;
	delay_ms(50);
	LCD_RST_CLR;
	delay_ms(1000);
	LCD_RST_SET;
	delay_ms(50);
}

/***************************************************************
 * Name:	 lcd_direction()
 * Input : direction:屏幕旋转方向
 * Output: void
 * Return: void
 * Author: hwl
 * Revise: V1.0
 * Description: 屏幕屏幕方向
 ***************************************************************/
static void lcd_direction(const uint8_t direction)
{ 
	g_lcd_param.setxcmd=0x2A;
	g_lcd_param.setycmd=0x2B;
	g_lcd_param.wramcmd=0x2C;
	g_lcd_param.rramcmd=0x2E;
	g_lcd_param.dir = direction%4;

	switch(g_lcd_param.dir)
	{
		case 0:	//0度
		{
			g_lcd_param.width=LCD_W;
			g_lcd_param.height=LCD_H;
			lcd_w_reg(0x36,(1<<3)|(0<<6)|(0<<7));					//BGR==1,MY==0,MX==0,MV==0
			break;
		}
		case 1:	//1-90度
		{
			g_lcd_param.width=LCD_H;
			g_lcd_param.height=LCD_W;
			lcd_w_reg(0x36,(1<<3)|(0<<7)|(1<<6)|(1<<5));	//BGR==1,MY==1,MX==0,MV==1
			break;
		}
		case 2:	//2-180度
		{
			g_lcd_param.width=LCD_W;
			g_lcd_param.height=LCD_H;
			lcd_w_reg(0x36,(1<<3)|(1<<6)|(1<<7));					//BGR==1,MY==0,MX==0,MV==0
			break;
		}
		case 3:	//3-270度
		{
			g_lcd_param.width=LCD_H;
			g_lcd_param.height=LCD_W;
			lcd_w_reg(0x36,(1<<3)|(1<<7)|(1<<5));					//BGR==1,MY==1,MX==0,MV==1
			break;
		}
		default:
		{
			break;
		}
	}
}

/***************************************************************
 * Name:	 drv_lcd_init()
 * Input : void
 * Output: void
 * Return: -1:失败
 * Author: hwl
 * Revise: V1.0
 * Description: lcd初始化
 ***************************************************************/
int drv_lcd_init(void)
{
 	lcd_reset();
	
	if(lcd_read_id() != 0x9341)
	{
		return -1;
	}

	lcd_w_cmd(0xCF);
	lcd_w_data(0x00);
	lcd_w_data(0xC1);
	lcd_w_data(0x30);
 
	lcd_w_cmd(0xED);
	lcd_w_data(0x64);
	lcd_w_data(0x03);
	lcd_w_data(0X12);
	lcd_w_data(0X81);
 
	lcd_w_cmd(0xE8);
	lcd_w_data(0x85);
	lcd_w_data(0x00);
	lcd_w_data(0x78);

	lcd_w_cmd(0xCB);
	lcd_w_data(0x39);
	lcd_w_data(0x2C);
	lcd_w_data(0x00);
	lcd_w_data(0x34);
	lcd_w_data(0x02);
	
	lcd_w_cmd(0xF7);
	lcd_w_data(0x20);
 
	lcd_w_cmd(0xEA);
	lcd_w_data(0x00);
	lcd_w_data(0x00);

	lcd_w_cmd(0xC0);
	lcd_w_data(0x13);
 
	lcd_w_cmd(0xC1);
	lcd_w_data(0x13);
 
	lcd_w_cmd(0xC5);
	lcd_w_data(0x22);
	lcd_w_data(0x35);
 
	lcd_w_cmd(0xC7);
	lcd_w_data(0xBD);

	lcd_w_cmd(0x21);

	lcd_w_cmd(0x36);
	lcd_w_data(0x08);

	lcd_w_cmd(0xB6);
	lcd_w_data(0x0A);
	lcd_w_data(0xA2);

	lcd_w_cmd(0x3A);
	lcd_w_data(0x55); 

	lcd_w_cmd(0xF6);
	lcd_w_data(0x01);
	lcd_w_data(0x30);

	lcd_w_cmd(0xB1);
	lcd_w_data(0x00);
	lcd_w_data(0x1B);
 
	lcd_w_cmd(0xF2);
	lcd_w_data(0x00);
 
	lcd_w_cmd(0x26);
	lcd_w_data(0x01);
 
	lcd_w_cmd(0xE0);
	lcd_w_data(0x0F);
	lcd_w_data(0x35);
	lcd_w_data(0x31);
	lcd_w_data(0x0B);
	lcd_w_data(0x0E);
	lcd_w_data(0x06);
	lcd_w_data(0x49);
	lcd_w_data(0xA7);
	lcd_w_data(0x33);
	lcd_w_data(0x07);
	lcd_w_data(0x0F);
	lcd_w_data(0x03);
	lcd_w_data(0x0C);
	lcd_w_data(0x0A);
	lcd_w_data(0x00);
 
	lcd_w_cmd(0XE1);
	lcd_w_data(0x00);
	lcd_w_data(0x0A);
	lcd_w_data(0x0F);
	lcd_w_data(0x04);
	lcd_w_data(0x11);
	lcd_w_data(0x08);
	lcd_w_data(0x36);
	lcd_w_data(0x58);
	lcd_w_data(0x4D);
	lcd_w_data(0x07);
	lcd_w_data(0x10);
	lcd_w_data(0x0C);
	lcd_w_data(0x32);
	lcd_w_data(0x34);
	lcd_w_data(0x0F);

	lcd_w_cmd(0x11);
	delay_ms(120);
	lcd_w_cmd(0x29);

	lcd_direction(0);	//设置LCD显示方向

	return 0;
}
