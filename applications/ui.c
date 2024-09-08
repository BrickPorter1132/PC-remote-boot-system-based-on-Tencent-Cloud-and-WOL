#include "ui.h"
#include <rtthread.h>
#include <rthw.h>
#include <u8g2_port.h>
#include "drv_common.h"
#include "ntp.h"
#include "file_config.h"
#include "common_param.h"

#define MAX_PAGE 5

static rt_sem_t ui_sem;
static u8g2_t u8g2;
static struct config_info config;

void oled_init(void)
{
    u8g2_Setup_ssd1306_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_4wire_sw_spi, u8x8_gpio_and_delay_rtthread);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_SPI_CLOCK, OLED_SPI_PIN_CLK);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_SPI_DATA, OLED_SPI_PIN_MOSI);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_CS, OLED_SPI_PIN_CS);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_DC, OLED_SPI_PIN_DC);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_RESET, OLED_SPI_PIN_RES);

//    u8g2_Setup_ssd1309_i2c_128x64_noname0_f( &u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_rtthread);
//    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_DATA, OLED_IIC_SDA);
//    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_CLOCK, OLED_IIC_SCL);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    u8g2_SetFont(&u8g2, u8g2_font_6x12_tf);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
}

void oled_show_system_boot(char *str)
{
    static unsigned char line_index = 0;

    //此函数的y坐标以字体的左下为原点，TMD的与众不同
    u8g2_DrawStr(&u8g2, 0, 12 * line_index + 11, str);
    line_index++;
    u8g2_SendBuffer(&u8g2);
}

#define PAGE_MAX   3

static void ui_thread_body(void *param)
{
    rt_device_t rtc_dev;
    time_t time;
    struct tm time_info;
    char str[10];
    rt_ubase_t page_index = 0;
    rt_ubase_t page_index_bak = 0;
    unsigned char page_changed_flag = 0;

    rtc_dev = rt_device_find("rtc");

    while(1)
    {
        if(rt_mb_recv(page_mail,  &page_index, 20) == RT_EOK)
        {
            page_changed_flag = 1;
            if(page_index == 100)
            {
                if(page_index_bak == PAGE_MAX)
                    page_index = 0;
                else
                    page_index = page_index_bak + 1;
            }
        }

        if(page_index == 0)
        {
            rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
            localtime_r(&time, &time_info);
            rt_sprintf(str, "%02d:%02d:%02d\r\n", time_info.tm_hour, time_info.tm_min, time_info.tm_sec);

            u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tf);
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 20, 40, str);
            u8g2_SendBuffer(&u8g2);
        }
        else if(page_changed_flag == 1)
        {
            if(page_index == 0)
                continue;

            u8g2_SetFont(&u8g2, u8g2_font_10x20_tf);
            u8g2_ClearBuffer(&u8g2);
            if(page_index == 1)
            {
                //12864 128 - 40 = 88
                u8g2_DrawStr(&u8g2, 44, 20, "WIFI");
                u8g2_SetFont(&u8g2, u8g2_font_7x14_tf);

                if(init_from_file("config.txt", &config) == RT_ERROR)
                    continue;
                rt_sprintf(str, "ssid:%s", config.wifi_name);
                u8g2_DrawStr(&u8g2, 0, 34, str);
                rt_sprintf(str, "passwd:%s", config.wifi_password);
                u8g2_DrawStr(&u8g2, 0, 48, str);
            }
            else if(page_index == 2)
            {
                u8g2_DrawStr(&u8g2, 49, 20, "MAC");
                u8g2_SetFont(&u8g2, u8g2_font_7x14_tf);

                if(init_from_file("config.txt", &config) == RT_ERROR)
                    continue;
                rt_sprintf(str, "%s", config.mac_address);
                u8g2_DrawStr(&u8g2, 0, 34, str);
            }
            else if(page_index == 3)
            {
//                u8g2_DrawStr(&u8g2, 4, 20, "ABOUT AUTHOR");
//                u8g2_SetFont(&u8g2, u8g2_font_7x14_tf);
                u8g2_DrawXBMP(&u8g2, 31, 0, 64, 64, qr_info);
            }
            u8g2_SendBuffer(&u8g2);
            page_changed_flag = 0;
        }

        page_index_bak = page_index;
    }
}

void ui_show(void)
{
    rt_thread_t ui_thread;

    ui_thread = rt_thread_create("ui_thread", ui_thread_body, RT_NULL, 2048, 10, 10);
    rt_thread_startup(ui_thread);
}

void ui_init(void)
{
    ui_sem = rt_sem_create("ui sem", 0, RT_IPC_FLAG_FIFO);
    ui_show();
}

static void ui_test(int argc, char *argv[])
{
    unsigned char x;
    unsigned char y;
    unsigned char str[20];

    x = atoi(argv[1]);
    y = atoi(argv[2]);
    rt_sprintf(str, "%s", argv[3]);

    u8g2_ClearBuffer(&u8g2);

    u8g2_DrawStr(&u8g2, x, y, str);
    u8g2_SendBuffer(&u8g2);
}

MSH_CMD_EXPORT(ui_test, ui test)

static void draw_line(void)
{
    static unsigned char index = 0;

    u8g2_ClearBuffer(&u8g2);

    u8g2_DrawHLine(&u8g2, 0, index, 128);
    u8g2_SendBuffer(&u8g2);

    rt_kprintf("%d\r\n", index);
    index++;
}

MSH_CMD_EXPORT(draw_line, line)


static void get_time(void)
{
    time_t net_time;
    struct tm time_info;

    net_time =  ntp_get_local_time("time1.cloud.tencent.com");

    if(net_time == 0)
    {
        rt_kprintf("Time get failed\r\n");
        return;
    }

    localtime_r(&net_time, &time_info);

//    rt_kprintf("%04d-%02d-%02d %02d:%02d:%02d\r\n", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
//            time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
//    rt_kprintf("%s\r\n", ctime(&net_time));
//    rt_kprintf("%d\r\n", net_time);

}

MSH_CMD_EXPORT(get_time, print time from net)
