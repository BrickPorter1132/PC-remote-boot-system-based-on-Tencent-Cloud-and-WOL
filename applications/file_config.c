#include "file_config.h"
#include <rtthread.h>
#include <dfs_posix.h>

//传入地址不得以'/'开头
rt_err_t init_from_file(const char *path, struct config_info *config)
{
    char temp[STR_SIZE];
    int fd;
    int index = 0;
    int line_index = 0;
    char ch;

    rt_sprintf(temp, "%s%s", ROOT_DIR, path);

    fd = open(temp, O_RDONLY);
    if(fd < 0)
    {
        rt_kprintf("Read config file failed, please check or re-config...\r\n");
        return RT_ERROR;
    }

    while(read(fd, &ch, 1) == 1)
    {
        if(ch != '\n')
            temp[index++] = ch;
        else
        {
            //这里投机取巧，牺牲了空间
            rt_memcpy((char *)config + line_index * STR_SIZE, temp, index);
            *((char *)config + line_index * STR_SIZE + index) = '\0';
            index = 0;
            line_index++;
        }
    }

    close(fd);
    return RT_EOK;
}

//覆盖式写入
rt_err_t write_to_file(const char *path, struct config_info *info)
{
    char temp[50];
    int fd;

    rt_sprintf(temp, "%s%s", ROOT_DIR, path);
    //O_TRUNC删除文件原有内容
    fd = open(temp, O_RDWR | O_CREAT | O_TRUNC);
    if(fd < 0)
    {
        rt_kprintf("Write config file failed, please check or re-config...\r\n");
        return RT_ERROR;
    }

    write(fd, info->wifi_name, strlen(info->wifi_name));
    write(fd, "\n", 1);
    write(fd, info->wifi_password, strlen(info->wifi_password));
    write(fd, "\n", 1);
    write(fd, info->mqtt_username, strlen(info->mqtt_username));
    write(fd, "\n", 1);
    write(fd, info->mqtt_passwd, strlen(info->mqtt_passwd));
    write(fd, "\n", 1);
    write(fd, info->mac_address, strlen(info->mac_address));
    write(fd, "\n", 1);

    close(fd);
    return RT_EOK;
}
