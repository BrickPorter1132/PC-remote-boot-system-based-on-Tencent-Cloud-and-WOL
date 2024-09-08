#include "common_param.h"

rt_mailbox_t computer_status_mail = RT_NULL;
rt_sem_t computer_on_sem = RT_NULL;
rt_mailbox_t page_mail = RT_NULL;
unsigned char sd_status = 0;
unsigned char is_online = 0;

void common_param_init(void)
{
    computer_status_mail = rt_mb_create("pc_status", 10, RT_IPC_FLAG_PRIO);
    RT_ASSERT(computer_status_mail != RT_NULL);
    computer_on_sem = rt_sem_create("pc_on", 0, RT_IPC_FLAG_PRIO);
    RT_ASSERT(computer_on_sem != RT_NULL);
    page_mail = rt_mb_create("PAGE", 10, RT_IPC_FLAG_PRIO);
    RT_ASSERT(page_mail != RT_NULL);
}
