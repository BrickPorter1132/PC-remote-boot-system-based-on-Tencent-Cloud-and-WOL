#ifndef _FILE_CONFIG_H_
#define _FILE_CONFIG_H_

#include <rtthread.h>

#define ROOT_DIR    "/sdcard/"
#define STR_SIZE    100

struct config_info
{
    char wifi_name[STR_SIZE];
    char wifi_password[STR_SIZE];
    char mqtt_username[STR_SIZE];
    char mqtt_passwd[STR_SIZE];
    char mac_address[STR_SIZE];
};

rt_err_t init_from_file(const char *path, struct config_info *config);
rt_err_t write_to_file(const char *path, struct config_info *info);

#endif
