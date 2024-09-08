#include "device_config.h"
#include "file_config.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

enum cmd
{
    CMD_NULL = 0,
    WIFI_CMD,
    MQTT_CMD,
    MAC_CMD
};

#define ESC_KEY                 0x1B
#define BACKSPACE_KEY           0x08
#define DELECT_KEY              0x7F

#define CONSOLE_FIFO_SIZE       256

static rt_uint8_t cur_cmd;
static struct rt_semaphore rx_notice;
static struct rt_ringbuffer *console_rx_fifo = RT_NULL;
static struct config_info config;

static rt_err_t (*console_rx_ind_bak)(rt_device_t dev, rt_size_t size) = RT_NULL;

static char console_getchar(void)
{
    char ch;

    rt_sem_take(&rx_notice, RT_WAITING_FOREVER);
    rt_ringbuffer_getchar(console_rx_fifo, (rt_uint8_t *)&ch);

    return ch;
}

rt_err_t console_rx_ind(rt_device_t dev, rt_size_t size)
{
    uint8_t ch;
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        if (rt_device_read(dev, 0, &ch, 1))
        {
            rt_ringbuffer_put_force(console_rx_fifo, &ch, 1);
            rt_sem_release(&rx_notice);
        }
    }

    return RT_EOK;
}

void guide_info_print(void)
{
    rt_kprintf("\r\n");
    rt_kprintf("********************\r\n");
    rt_kprintf("*1.Wifi     change *\r\n");
    rt_kprintf("*2.MQTT     change *\r\n");
    rt_kprintf("*3.MAC      change *\r\n");
    rt_kprintf("******ESC exit******\r\n");
}

void cmd_handle(char *data, rt_size_t data_len)
{
    //顶层为0，输入数字后变为1
    static unsigned char cur_lvl = 0;
    static struct config_info config_temp;

    if(cur_cmd == CMD_NULL)
    {
        if((data_len > 1) || (*data > '3') || (*data < '1'))
        {
            rt_kprintf("Error, please input Number 1-3\r\n");
            guide_info_print();
            return;
        }

        cur_cmd = *data - '0';
        cur_lvl = 1;
        switch(cur_cmd)
        {
            case WIFI_CMD:
            {
                rt_kprintf("please input wifi name: ");
                break;
            }
            case MQTT_CMD:
            {
                rt_kprintf("please input mqtt user name: ");
                break;
            }
            case MAC_CMD:
            {
                rt_kprintf("please input MAC address: ");
                break;
            }
        }
    }
    else if(cur_cmd == WIFI_CMD)
    {
        if(cur_lvl == 1)
        {
            rt_memcpy(config_temp.wifi_name, data, data_len);
            config_temp.wifi_name[data_len] = '\0';

            rt_kprintf("please input wifi password: ");
            cur_lvl++;
        }
        else if(cur_lvl == 2)
        {
            rt_memcpy(config_temp.wifi_password, data, data_len);
            config_temp.wifi_password[data_len] = '\0';

            rt_memcpy(config.wifi_name, config_temp.wifi_name, sizeof(config.wifi_name));
            rt_memcpy(config.wifi_password, config_temp.wifi_password, sizeof(config.wifi_password));
            write_to_file("config.txt", &config);
            cur_lvl = 0;
            cur_cmd = CMD_NULL;
            rt_kprintf("wifi info saved!\r\n");
            guide_info_print();
        }

    }
    else if(cur_cmd == MQTT_CMD)
    {
        if(cur_lvl == 1)
        {
            rt_memcpy(config_temp.mqtt_username, data, data_len);
            config_temp.mqtt_username[data_len] = '\0';

            rt_kprintf("please input mqtt password: ");
            cur_lvl++;
        }
        else if(cur_lvl == 2)
        {
            rt_memcpy(config_temp.mqtt_passwd, data, data_len);
            config_temp.mqtt_passwd[data_len] = '\0';

            rt_memcpy(config.mqtt_username, config_temp.mqtt_username, sizeof(config.mqtt_username));
            rt_memcpy(config.mqtt_passwd, config_temp.mqtt_passwd, sizeof(config.mqtt_passwd));

            write_to_file("config.txt", &config);
            cur_lvl = 0;
            cur_cmd = CMD_NULL;
            rt_kprintf("mqtt info saved!\r\n");
            guide_info_print();
        }
    }
    else if(cur_cmd == MAC_CMD)
    {
        if(
                (data_len == 17) &&
                (*(data + 0) <= 'f') && (*(data + 0) >= '0') &&
                (*(data + 1) <= 'f') && (*(data + 1) >= '0') &&
                (*(data + 3) <= 'f') && (*(data + 3) >= '0') &&
                (*(data + 4) <= 'f') && (*(data + 4) >= '0') &&
                (*(data + 6) <= 'f') && (*(data + 6) >= '0') &&
                (*(data + 7) <= 'f') && (*(data + 7) >= '0') &&
                (*(data + 9) <= 'f') && (*(data + 9) >= '0') &&
                (*(data + 10) <= 'f') && (*(data + 10) >= '0') &&
                (*(data + 12) <= 'f') && (*(data + 12) >= '0') &&
                (*(data + 13) <= 'f') && (*(data + 13) >= '0') &&
                (*(data + 15) <= 'f') && (*(data + 15) >= '0') &&
                (*(data + 16) <= 'f') && (*(data + 16) >= '0')
          )
        {
            rt_memcpy(config.mac_address, data, data_len);
            config.mac_address[data_len] = '\0';
            write_to_file("config.txt", &config);

            cur_cmd = CMD_NULL;
            rt_kprintf("MAC address saved!\r\n");
            guide_info_print();
        }
        else
        {
            rt_kprintf("Error, please re-input: ");
        }
    }
}

void config_init(void)
{
    rt_base_t int_lvl;
    rt_device_t console;
    rt_int8_t ch;
    rt_size_t cur_line_len = 0;
    char cur_line[128] = {0};

    cur_cmd = CMD_NULL;

    //获取本机文件配置
    init_from_file("config.txt", &config);

    int_lvl = rt_hw_interrupt_disable();

    console = rt_console_get_device();
    console_rx_ind_bak = console->rx_indicate;
    rt_device_set_rx_indicate(console, console_rx_ind);

    rt_hw_interrupt_enable(int_lvl);

    rt_sem_init(&rx_notice, "input rx notice", 0, RT_IPC_FLAG_FIFO);

    console_rx_fifo = rt_ringbuffer_create(CONSOLE_FIFO_SIZE);

    guide_info_print();

    while (1)
    {
        ch = console_getchar();
        if(ch == ESC_KEY && cur_cmd != CMD_NULL)
        {
            cur_cmd = CMD_NULL;
            guide_info_print();
            continue;
        }
        else if(ch == ESC_KEY)
            break;

        if (ch == BACKSPACE_KEY || ch == DELECT_KEY)
        {
            if (cur_line_len)
            {
                cur_line[--cur_line_len] = 0;
                rt_kprintf("\b \b");
            }
            continue;
        }
        else if (ch == '\r' || ch == '\n')
        {
            //没有输入的时候，不允许回车换行
            if (cur_line_len)
            {
                rt_kprintf("\n");
                cmd_handle(cur_line, cur_line_len);
            }
            cur_line_len = 0;
        }
        else
        {
            rt_kprintf("%c", ch);
            cur_line[cur_line_len++] = ch;
        }
    }
}

void config_deinit(void)
{
    rt_base_t int_lvl;
    rt_device_t console;

    int_lvl = rt_hw_interrupt_disable();

    console = rt_console_get_device();
    rt_device_set_rx_indicate(console, console_rx_ind_bak);

    rt_hw_interrupt_enable(int_lvl);

    rt_sem_detach(&rx_notice);
    rt_ringbuffer_destroy(console_rx_fifo);
}

static void system_config(void)
{
    config_init();
    config_deinit();
}

MSH_CMD_EXPORT(system_config, config environment to make system run)
