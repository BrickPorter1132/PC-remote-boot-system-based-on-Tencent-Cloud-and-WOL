#include <rtthread.h>
#include <wlan_mgnt.h>
#include "file_config.h"
#include "ntp.h"
#include "ui.h"
#include "mqtt.h"
#include "common_param.h"
#include "boot_check.h"
#include "wol.h"
#include <rtdevice.h>
#include "drv_common.h"

#define WIFI_CONNECT_DELAY  1000
#define KEY_PIN GET_PIN(H, 4)

static struct config_info config;

rt_err_t config_check(void)
{
    return init_from_file("config.txt", &config);
}

rt_err_t wifi_connect(void)
{
    rt_err_t ret;

    while(rt_wlan_connect(config.wifi_name, config.wifi_password) == -RT_EIO)
        rt_thread_mdelay(WIFI_CONNECT_DELAY);

    for(int i = 0; i < 10; i++)
    {
        ret = rt_wlan_connect(config.wifi_name, config.wifi_password);
        if(ret == RT_EOK)
        {
            rt_kprintf("wifi connect successed!\r\n");
            return ret;
        }
        else
            rt_thread_mdelay(WIFI_CONNECT_DELAY);
    }
    rt_kprintf("wifi connect failed!\r\n");
    return RT_ERROR;
}

static void key_check(void *param)
{
    rt_base_t key_value = PIN_HIGH;
    rt_base_t key_value_bak = PIN_HIGH;

    while(1)
    {
        key_value = rt_pin_read(KEY_PIN);

        if(key_value != key_value_bak && key_value == PIN_LOW)
        {
            rt_mb_send(page_mail, 100);
        }
        key_value_bak = key_value;
        rt_thread_mdelay(10);
    }
}

void system_boot(void)
{
    unsigned char index = 0;
    oled_init();

    while(1)
    {
        if(sd_status == 0)
            index++;
        else
            break;
        if(index == 100)
        {
            oled_show_system_boot("Please check sd card");
            return;
        }
        rt_thread_mdelay(50);
    }

    if(config_check() != RT_EOK)
    {
        rt_kprintf("ERROR Please config the system first!\r\n");
        oled_show_system_boot("Read config failed");
        return;
    }
    oled_show_system_boot("Read config successed");
    if(wifi_connect() != RT_EOK)
    {
        oled_show_system_boot("Connect wifi failed");
        return;
    }
    oled_show_system_boot("WIFI connected");

    rt_thread_mdelay(1000);
    //更新时间
    while(ntp_sync_to_rtc(RT_NULL) == 0)
        rt_thread_mdelay(100);

    mqtt_start(config.mqtt_username, config.mqtt_passwd);
    index = 0;
    while(is_online == 0)
    {
        rt_thread_mdelay(100);
        index++;
        if(index == 100)
        {
            rt_kprintf("ERROR Please check MQTT status!\r\n");
            oled_show_system_boot("Connect MQTT failed");
            return;
        }
    }
    oled_show_system_boot("MQTT connected");
    oled_show_system_boot("System running...");

    common_param_init();
    create_computer_status_report_thread();
    create_computer_status_check_thread();
    create_wol_thread();

    ui_init();

    rt_thread_t key_thread;
    rt_pin_mode(KEY_PIN, PIN_MODE_INPUT);
    key_thread = rt_thread_create("key_check", key_check, RT_NULL, 4096, 10, 10);
    RT_ASSERT(key_thread);
    rt_thread_startup(key_thread);
}

MSH_CMD_EXPORT(system_boot, Reboot system when config failed)
