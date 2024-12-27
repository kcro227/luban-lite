#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <finsh.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>
#include "aic_core.h"
#include "aic_hal_gpio.h"
#include "./lcd_init.h"
#include "./lcd.h"
#include "./pic.h"


#define THREAD_PRIORITY         25      // 线程优先级
#define THREAD_STACK_SIZE       4096    // 线程大小
#define THREAD_TIMESLICE        10      // 时间片

#define QPSI_BUS_NANE       "qspi3"     //  qspi总线名称
#define QPSI_DEVICE_NAME    "lcd"       //  qspi设备lcd名称


static struct rt_qspi_device *qspi_device = RT_NULL; // qspi设备结构体
static rt_thread_t lcd_thread = RT_NULL; // 线程控制块

/**********************************************************
 * 函 数 名 称：lcd_qspi_attach
 * 函 数 功 能：挂载qspi的lcd设备
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
static int lcd_qspi_attach(void)
{
    // 为spi结构体申请一片空间
    qspi_device = (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if(RT_NULL == qspi_device)
    {
        LOG_E("Failed to malloc the qspi device.");
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);
        return -RT_ENOMEM;
    }    
    rt_kprintf("malloc the qspi succeed.\r\n");

    qspi_device->enter_qspi_mode = RT_NULL;
    qspi_device->exit_qspi_mode = RT_NULL;
    qspi_device->config.qspi_dl_width = 1;

    int ret = rt_spi_bus_attach_device(&qspi_device->parent, QPSI_DEVICE_NAME, QPSI_BUS_NANE, RT_NULL);
    if(ret != RT_EOK)
    {
        LOG_E("Failed to rt_spi_bus_attach_device.");
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return ret;
    }

    return RT_EOK;

}


/**********************************************************
 * 函 数 名 称：lcd_qspi_init
 * 函 数 功 能：配置qspi进行初始化
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
static int lcd_qspi_init(void)
{
    struct rt_qspi_device *g_qspi;
    struct rt_qspi_configuration qspi_cfg;
    struct rt_device *dev;
    char *name;
    int ret = 0;

    name = QPSI_DEVICE_NAME;

    g_qspi = (struct rt_qspi_device *)rt_device_find(name);
    if (!g_qspi) 
    {
        LOG_E("Failed to get device in name %s\n", name);
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return -1;
    }

    dev = (struct rt_device *)g_qspi;
    if (dev->type != RT_Device_Class_SPIDevice)
    {
        g_qspi = NULL;

        LOG_E("%s is not SPI device.\n", name);
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return -1;
    }

    rt_memset(&qspi_cfg, 0, sizeof(qspi_cfg));

    qspi_cfg.qspi_dl_width = 1;     // QSPI 总线位宽，单线模式 1 位、双线模式 2 位，4 线模式 4 位
    qspi_cfg.parent.mode = RT_SPI_MASTER | RT_SPI_MODE_2 | RT_SPI_MSB;
    qspi_cfg.parent.data_width = 8;
    qspi_cfg.parent.max_hz = 40 * 1000 * 1000;   // 20M
    ret = rt_qspi_configure(g_qspi, &qspi_cfg);
    if (ret < 0) 
    {
        LOG_E("qspi configure failure.");
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return ret;
    }

    return RT_EOK;
}


// 线程入口函数
static void lcd_thread_entry(void *param)
{
    float t = 0;

    LCD_Init();//屏幕初始化
    rt_kprintf("[ LCD_Init ] succeed.\r\n");

    LCD_Fill(0,0,LCD_W,LCD_H,WHITE);//清全屏为白色
    rt_kprintf("[ LCD_Fill ] succeed.\r\n");
    
    while(1)
    {           
        if(t >= 99.88)
            t = 0;

        LCD_ShowChinese(40,0,(uint8_t *)"中电子",RED,WHITE,32,0);
        LCD_ShowString(10,33,(uint8_t *)"LCD_W:",RED,WHITE,32,0);
        LCD_ShowIntNum(106,33,LCD_W,3,RED,WHITE,32);
        LCD_ShowString(10,66,(uint8_t *)"LCD_H:",RED,WHITE,32,0);
        LCD_ShowIntNum(106,66,LCD_H,3,RED,WHITE,32);
        LCD_ShowFloatNum1(10,99,t,4,RED,WHITE,32);
        t+=0.1;
        LCD_ShowPicture(160,95,40,40,gImage_1);
    
        rt_thread_mdelay(500);
    }
}



/**********************************************************
 * 函 数 名 称：test_lckfb_1_69_color_screen
 * 函 数 功 能：1.69寸彩屏测试程序
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
static int test_lckfb_1_69_color_screen(void)
{
    int ret = 0;

    ret = lcd_qspi_attach();
    if(ret != RT_EOK)
    {
        LOG_E("[ lcd_qspi_attach ] failure.");
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return RT_ERROR; 
    }
    rt_kprintf("[ lcd_qspi_attach ] succeed.\r\n");

    ret = lcd_qspi_init();
    if(ret != RT_EOK)
    {
        LOG_E("[ lcd_qspi_init ] failure.");
        LOG_E("file: %s",__FILE__);
        LOG_E("line: %d",__LINE__);

        return RT_ERROR; 
    }
    rt_kprintf("[ lcd_qspi_init ] succeed.\r\n");

    /* 创建线程，名称是 lcd_thread，入口是 lcd_thread_entry */
    lcd_thread = rt_thread_create("lcd_thread",
                            lcd_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    /* 如果获得线程控制块，启动这个线程 */
    if (lcd_thread != RT_NULL)
        rt_thread_startup(lcd_thread);

    return RT_EOK;


}
MSH_CMD_EXPORT(test_lckfb_1_69_color_screen, color screen test);





