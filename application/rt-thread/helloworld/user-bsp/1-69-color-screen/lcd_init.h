#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include <stdio.h>
#include <inttypes.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_hal_gpio.h"

#define USE_HORIZONTAL 0  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240
#define LCD_H 280

#else
#define LCD_W 280
#define LCD_H 240
#endif


#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

//-----------------LCD端口移植---------------- 
#define LCD_RES     rt_pin_get("PC.7")
#define LCD_DC      rt_pin_get("PE.16")
#define LCD_BLK     rt_pin_get("PE.14")
#define LCD_CS      rt_pin_get("PC.9")  

//-----------------LCD端口定义---------------- 

#define LCD_RES_Clr()  rt_pin_write(LCD_RES, 0)//RES
#define LCD_RES_Set()  rt_pin_write(LCD_RES, 1)

#define LCD_DC_Clr()   rt_pin_write(LCD_DC, 0)//DC
#define LCD_DC_Set()   rt_pin_write(LCD_DC, 1)
 		     
#define LCD_BLK_Clr()  rt_pin_write(LCD_BLK, 0)//BLK
#define LCD_BLK_Set()  rt_pin_write(LCD_BLK, 1)

#define LCD_CS_Clr()  rt_pin_write(LCD_CS, 0)//CS
#define LCD_CS_Set()  rt_pin_write(LCD_CS, 1)



void delay_us(int us);
void delay_ms(int ms);
void LCD_GPIO_Init(void);//初始化GPIO
void LCD_Writ_Bus(u8 dat);//模拟SPI时序
void LCD_WR_DATA8(u8 dat);//写入一个字节
void LCD_WR_DATA(u16 dat);//写入两个字节
void LCD_WR_REG(u8 dat);//写入一个指令
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);//设置坐标函数
void LCD_Init(void);//LCD初始化
#endif




