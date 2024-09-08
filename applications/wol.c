#include "wol.h"
#include <sys/socket.h>
#include "file_config.h"
#include "netdev.h"
#include <stdlib.h>
#include <rtthread.h>
#include "common_param.h"

void wol_thread(void)
{
    unsigned char data[102];
    struct sockaddr_in pc_addr;
    int sockfd = -1;
    socklen_t opt_val = 1;
    struct config_info config;
    rt_err_t ret;
    unsigned char mac[6];
    unsigned long ip_addr;

    while(1)
    {
        rt_sem_take(computer_on_sem, RT_WAITING_FOREVER);

        rt_kprintf("send wol packet\r\n");

        rt_memset(config.mac_address, 0, sizeof(config.mac_address));
        ret = init_from_file("config.txt", &config);
        if(ret != RT_EOK)
        {
            rt_kprintf("read config file failed\r\n");
            continue;
        }

        rt_kprintf("%s\r\n", config.mac_address);
        for(int i = 0; i < sizeof(config.mac_address); i++)
        {
            if(config.mac_address[i] == '.')
                config.mac_address[i] = '\0';
        }
        mac[0] = (unsigned char)strtol(&config.mac_address[0], RT_NULL, 16);
        mac[1] = (unsigned char)strtol(&config.mac_address[3], RT_NULL, 16);
        mac[2] = (unsigned char)strtol(&config.mac_address[6], RT_NULL, 16);
        mac[3] = (unsigned char)strtol(&config.mac_address[9], RT_NULL, 16);
        mac[4] = (unsigned char)strtol(&config.mac_address[12], RT_NULL, 16);
        mac[5] = (unsigned char)strtol(&config.mac_address[15], RT_NULL, 16);
        rt_kprintf("%02x %02x %02x %02x %02x %02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        for(int i = 0; i < 6; i++)
            data[i] = 0xff;
        for(int i = 0; i < 16; i++)
        {
            for(int j = 0; j < 6; j++)
                data[6 + i * 6 + j] = mac[j];
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd == -1)
            rt_kprintf("sock create failed\r\n");
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&opt_val, sizeof(opt_val));

        ip_addr = netdev_default->ip_addr.addr & netdev_default->netmask.addr;
        ip_addr = (~netdev_default->netmask.addr) | ip_addr;

        pc_addr.sin_family = AF_INET;
        pc_addr.sin_port = htons(9);
        pc_addr.sin_addr.s_addr = ip_addr;
        rt_memset(&(pc_addr.sin_zero), 0, sizeof(pc_addr.sin_zero));

        sendto(sockfd, data, sizeof(data), 0, (struct sockaddr *)&pc_addr, sizeof(struct sockaddr));

        closesocket(sockfd);
    }


}

void create_wol_thread(void)
{
    rt_thread_t thread;

    thread = rt_thread_create("wol_send", wol_thread, RT_NULL, 2048, 25, 10);
    RT_ASSERT(thread != RT_NULL);
    rt_thread_startup(thread);
}
