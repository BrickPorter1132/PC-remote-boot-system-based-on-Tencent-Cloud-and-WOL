#include "boot_check.h"
#include <rtthread.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/sys.h>
#include <lwip/netif.h>
#include <lwip/etharp.h>
#include "common_param.h"

#define PING_TIMEOUT 100
static unsigned char find_mac[6] = {0x88, 0xae, 0xdd, 0x15, 0x60, 0x95};
ip4_addr_t computer_ip_addr = {.addr = 0};

int ping_and_find_ip(void)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    uint32_t base_ip;
    uint32_t subnet_mask;
    uint32_t ip;
    uint32_t num_addresses = 1;
    int sock;
    struct sockaddr_in to_addr;
    struct timeval timeout = {PING_TIMEOUT / 1000, (PING_TIMEOUT % 1000) * 1000}; // 设置超时时间
    char send_buf[64];
    char recv_buf[64];

    if (netif_default != RT_NULL)
    {
        ipaddr = netif_default->ip_addr;
        netmask = netif_default->netmask;
    }
    else
    {
        rt_kprintf("No network interface found.\n");
        return;
    }
    subnet_mask = netmask.addr;
    base_ip = ipaddr.addr & netmask.addr;

    // 计算子网中IP地址的数量
    for (uint32_t i = 0; i < (32 - __builtin_popcount(subnet_mask)); i++)
    {
        num_addresses *= 2;
    }

    // 创建原始套接字
    sock = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (sock < 0)
    {
        rt_kprintf("Failed to create socket\n");
        return -1;
    }

    // 设置超时时间
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 设置目标地址
    memset(&to_addr, 0, sizeof(to_addr));
    to_addr.sin_family = AF_INET;

    // 构造ICMP请求数据包
    struct icmp_echo_hdr *icmp_req = (struct icmp_echo_hdr *)send_buf;
    ICMPH_TYPE_SET(icmp_req, ICMP_ECHO);
    ICMPH_CODE_SET(icmp_req, 0);
    icmp_req->chksum = 0;
    icmp_req->id = lwip_htons(1);
    icmp_req->seqno = lwip_htons(1);
    icmp_req->chksum = inet_chksum(icmp_req, sizeof(send_buf));

    // 跳过网络地址和广播地址
    for (uint32_t i = 1; i < num_addresses - 1; i++)
    {
        ip = base_ip | (i << 24);
        to_addr.sin_addr.s_addr = ip;
        // 发送Ping请求
        if (lwip_sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0)
        {
            rt_kprintf("Failed to send Ping request\n");
        }
        else
        {
            // 等待接收Ping响应
            int recv_len = lwip_recv(sock, recv_buf, sizeof(recv_buf), 0);
        }

        computer_ip_addr = get_ip_from_mac(netif_default, find_mac);

        if(computer_ip_addr.addr != 0)
        {
            rt_kprintf("IP: %s found\r\n",ip4addr_ntoa(&computer_ip_addr));
            rt_kprintf("Computer has started\r\n");
            lwip_close(sock);
            return 0;
        }
    }
    // 关闭套接字
    lwip_close(sock);
    return -1;
}

MSH_CMD_EXPORT(ping_and_find_ip, ping_and_find_ip)

int ping(void)
{
    int sock;
    struct sockaddr_in to_addr;
    struct timeval timeout = {1000 / 1000, (1000 % 1000) * 1000}; // 设置超时时间
    char send_buf[64];
    char recv_buf[64];
    int ping_result = 0;

    // 创建原始套接字
    sock = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (sock < 0)
    {
        rt_kprintf("Failed to create socket\n");
        return 0;
    }

    // 设置超时时间
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 设置目标地址
    memset(&to_addr, 0, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    to_addr.sin_addr.s_addr = computer_ip_addr.addr;

    // 构造ICMP请求数据包
    struct icmp_echo_hdr *icmp_req = (struct icmp_echo_hdr *)send_buf;
    ICMPH_TYPE_SET(icmp_req, ICMP_ECHO);
    ICMPH_CODE_SET(icmp_req, 0);
    icmp_req->chksum = 0;
    icmp_req->id = lwip_htons(1);
    icmp_req->seqno = lwip_htons(1);
    icmp_req->chksum = inet_chksum(icmp_req, sizeof(send_buf));

    // 发送Ping请求
    if (lwip_sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0)
    {
        rt_kprintf("Failed to send Ping request\n");
    }
    else
    {
        // 等待接收Ping响应
        int recv_len = lwip_recv(sock, recv_buf, sizeof(recv_buf), 0);
        if (recv_len > 0)
        {
            struct ip_hdr *iphdr = (struct ip_hdr *)recv_buf;
            struct icmp_echo_hdr *icmp_hdr = (struct icmp_echo_hdr *)(recv_buf + (IPH_HL(iphdr) * 4));

            // 判断ICMP响应类型为回显应答
            if (icmp_hdr->type == ICMP_ER)
            {
                //rt_kprintf("Ping successful, received response from %s\n", target_ip);
                ping_result = 1;
            }
        }
        else
        {
            //rt_kprintf("Ping timeout, no response from %s\n", target_ip);
        }
    }

    // 关闭套接字
    lwip_close(sock);

    return ping_result;
}

void computer_status_check_thread(void *param)
{
    unsigned int computer_status = 0;
    unsigned int computer_status_bak = 1;

    if(computer_ip_addr.addr == 0)
    {
        while(1)
        {
            if(computer_status != computer_status_bak)
                rt_mb_send(computer_status_mail, computer_status);
            computer_status_bak = computer_status;
            if(ping_and_find_ip() == -1)
                rt_thread_mdelay(1000);
            else
            {
                computer_status = 1;
                break;
            }
        }
    }
    while(1)
    {
        if(computer_status != computer_status_bak)
            rt_mb_send(computer_status_mail, computer_status);
        computer_status_bak = computer_status;
        if(ping() == 1)
        {
            computer_status = 1;
        }
        else
        {
            computer_status = 0;
        }

        rt_thread_mdelay(1000);
    }
}

void create_computer_status_check_thread(void)
{
    rt_thread_t thread;

    thread = rt_thread_create("boot_check", computer_status_check_thread, RT_NULL, 2048, 25, 10);
    RT_ASSERT(thread != RT_NULL);
    rt_thread_startup(thread);
}
