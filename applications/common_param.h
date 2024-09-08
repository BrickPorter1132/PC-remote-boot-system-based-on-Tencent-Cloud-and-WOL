/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-08-31     Lenovo       the first version
 */
#ifndef APPLICATIONS_COMMON_PARAM_H_
#define APPLICATIONS_COMMON_PARAM_H_

#include <rtthread.h>

extern rt_mailbox_t computer_status_mail;
extern rt_mailbox_t page_mail;
extern rt_sem_t computer_on_sem;
extern unsigned char sd_status;
extern unsigned char is_online;

extern void common_param_init(void);



#endif /* APPLICATIONS_COMMON_PARAM_H_ */
