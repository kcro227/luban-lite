#include "./lcd_init.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_qspi.h>

#define LCD_DEVICE_NAME "lcd"

static struct rt_qspi_device *lcd_dev;

void delay_ms(int ms) { aicos_mdelay(ms); }
void delay_us(int us) { aicos_udelay(us); }

void LCD_GPIO_Init(void)
{
	/* 查找设备并获取设备句柄 */
	lcd_dev = (struct rt_qspi_device *)rt_device_find(LCD_DEVICE_NAME);
	if (!lcd_dev)
	{
		LOG_E("Qspi sample run failed! can't find %s device!\n", LCD_DEVICE_NAME);
		LOG_E("file: %s", __FILE__);
		LOG_E("line: %d", __LINE__);
	}

	rt_pin_mode(LCD_RES, PIN_MODE_OUTPUT); // 配置引脚为输出模式
	rt_pin_mode(LCD_DC, PIN_MODE_OUTPUT);  // 配置引脚为输出模式
	rt_pin_mode(LCD_BLK, PIN_MODE_OUTPUT); // 配置引脚为输出模式
	rt_pin_mode(LCD_CS, PIN_MODE_OUTPUT);  // 配置引脚为输出模式

	LCD_RES_Set();
	LCD_DC_Set();
	LCD_BLK_Set();
	LCD_CS_Set();

	rt_kprintf("LCD_GPIO_Init end!!\n");
}

/******************************************************************************
	  函数说明：LCD串行数据写入函数
	  入口数据：dat  要写入的串行数据
	  返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat)
{
	struct rt_qspi_message msg;

	rt_memset(&msg, 0, sizeof(msg));

	msg.instruction.content = 0;	/* 指令内容 */
	msg.instruction.qspi_lines = 0; /* 指令模式，单线模式 1 位、双线模式 2 位，4 线模式 4 位 */

	msg.alternate_bytes.content = 0;	/* 地址/交替字节 内容 */
	msg.alternate_bytes.size = 0;		/* 地址/交替字节 长度 */
	msg.alternate_bytes.qspi_lines = 0; /* 地址/交替字节 模式，单线模式 1 位、双线模式 2 位，4 线模式 4 位 */

	msg.dummy_cycles = 0;	 /* 空指令周期阶段 */
	msg.qspi_data_lines = 1; /*  QSPI 总线位宽 */

	// 传输一条消息，开始发送数据时片选选中，函数返回时释放片选。
	msg.parent.send_buf = &dat;	   /* 发送缓冲区指针 */
	msg.parent.recv_buf = RT_NULL; /* 接收缓冲区指针 */
	msg.parent.length = 1;		   /* 发送 / 接收 数据字节数 */
	msg.parent.next = RT_NULL;	   /* 指向继续发送的下一条消息的指针 */
	msg.parent.cs_take = 1;		   /* 片选选中 */
	msg.parent.cs_release = 1;	   /* 释放片选 */

	rt_spi_take_bus((struct rt_spi_device *)lcd_dev);

	LCD_CS_Clr();

	int ret = rt_qspi_transfer_message(lcd_dev, &msg);
	;
	if (ret != 1)
	{
		LOG_E("rt_qspi_transfer_message failed!!");
	}

	LCD_CS_Set();

	rt_spi_release_bus((struct rt_spi_device *)lcd_dev);
}

/******************************************************************************
	  函数说明：LCD写入数据
	  入口数据：dat 写入的数据
	  返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}

/******************************************************************************
	  函数说明：LCD写入数据
	  入口数据：dat 写入的数据
	  返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
	LCD_Writ_Bus(dat >> 8);
	LCD_Writ_Bus(dat);
}

/******************************************************************************
	  函数说明：LCD写入命令
	  入口数据：dat 写入的命令
	  返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr(); // 写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set(); // 写数据
}

/******************************************************************************
	  函数说明：设置起始和结束地址
	  入口数据：x1,x2 设置列的起始和结束地址
				y1,y2 设置行的起始和结束地址
	  返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1, u16 y1, u16 x2, u16 y2)
{
	if (USE_HORIZONTAL == 0)
	{
		LCD_WR_REG(0x2a); // 列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b); // 行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c); // 储存器写
	}
	else if (USE_HORIZONTAL == 1)
	{
		LCD_WR_REG(0x2a); // 列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b); // 行地址设置
		LCD_WR_DATA(y1 + 20);
		LCD_WR_DATA(y2 + 20);
		LCD_WR_REG(0x2c); // 储存器写
	}
	else if (USE_HORIZONTAL == 2)
	{
		LCD_WR_REG(0x2a); // 列地址设置
		LCD_WR_DATA(x1 + 20);
		LCD_WR_DATA(x2 + 20);
		LCD_WR_REG(0x2b); // 行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c); // 储存器写
	}
	else
	{
		LCD_WR_REG(0x2a); // 列地址设置
		LCD_WR_DATA(x1 + 20);
		LCD_WR_DATA(x2 + 20);
		LCD_WR_REG(0x2b); // 行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c); // 储存器写
	}
}

void LCD_Init(void)
{
	LCD_GPIO_Init(); // 初始化GPIO

	LCD_RES_Clr(); // 复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(50);
	LCD_BLK_Set(); // 打开背光
	delay_ms(100);

	LCD_WR_REG(0x11);
	delay_ms(120);

	LCD_WR_REG(0x36);
	if (USE_HORIZONTAL == 0)
		LCD_WR_DATA8(0x48);
	else if (USE_HORIZONTAL == 1)
		LCD_WR_DATA8(0xC0);
	else if (USE_HORIZONTAL == 2)
		LCD_WR_DATA8(0x70);
	else
		LCD_WR_DATA8(0xA0);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x55); // LCD_WR_DATA(0x66);

	LCD_WR_REG(0xF0); // Command Set Control
	LCD_WR_DATA8(0xC3);

	LCD_WR_REG(0xF0);
	LCD_WR_DATA8(0x96);

	LCD_WR_REG(0xB4);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xB7);
	LCD_WR_DATA8(0xC6);

	// LCD_WR_REG(0xB9);
	// LCD_WR_DATA(0x02);
	// LCD_WR_DATA(0xE0);

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x80);
	LCD_WR_DATA8(0x45);

	LCD_WR_REG(0xC1);
	LCD_WR_DATA8(0x13); // 18  //00

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0xA7);

	LCD_WR_REG(0xC5);
	LCD_WR_DATA8(0x0A);

	LCD_WR_REG(0xE8);
	LCD_WR_DATA8(0x40);
	LCD_WR_DATA8(0x8A);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x29);
	LCD_WR_DATA8(0x19);
	LCD_WR_DATA8(0xA5);
	LCD_WR_DATA8(0x33);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x08);
	LCD_WR_DATA8(0x0F);
	LCD_WR_DATA8(0x06);
	LCD_WR_DATA8(0x06);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x30);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x47);
	LCD_WR_DATA8(0x17);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2B);
	LCD_WR_DATA8(0x31);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x09);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x2F);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x47);
	LCD_WR_DATA8(0x38);
	LCD_WR_DATA8(0x15);
	LCD_WR_DATA8(0x16);
	LCD_WR_DATA8(0x2C);
	LCD_WR_DATA8(0x32);

	LCD_WR_REG(0xF0);
	LCD_WR_DATA8(0x3C);

	LCD_WR_REG(0xF0);
	LCD_WR_DATA8(0x69);

	delay_ms(120);

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x29);
}
