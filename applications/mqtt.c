#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <rtthread.h>
#include "mqtt.h"
#include "cJSON.h"
#include "common_param.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME    "mqtt.own"
#define DBG_LEVEL           DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "paho_mqtt.h"

/**
 * MQTT URI farmat:
 * domain mode
 * tcp://iot.eclipse.org:1883
 *
 * ipv4 mode
 * tcp://192.168.10.1:1883
 * ssl://192.168.10.1:1884
 *
 * ipv6 mode
 * tcp://[fe80::20c:29ff:fe9a:a07e]:1883
 * ssl://[fe80::20c:29ff:fe9a:a07e]:1884
 */
#define MQTT_URI                "tcp://AFP2CEYM1W.iotcloud.tencentdevices.com:1883"
#define MQTT_SUBTOPIC           "$thing/down/property/OJ2O020V2M/dev_001"
#define MQTT_PUBTOPIC           "$thing/up/property/OJ2O020V2M/dev_001"
#define MQTT_WILLMSG            "Goodbye!"

/* define MQTT client context */
static MQTTClient client;
static int is_started = 0;

static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data)
{
    cJSON *cjson_data;
    char *str = NULL;
    unsigned char ctl_status = 0;
    rt_ubase_t page = 0;

    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub callback: %.*s %.*s",
               msg_data->topicName->lenstring.len,
               msg_data->topicName->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);

    cjson_data = cJSON_Parse(msg_data->message->payload);
    if(!cjson_data)
    {
        rt_kprintf("json phase error%s\r\n", cjson_data);
        cJSON_Delete(cjson_data);
        return;
    }
    cJSON *cparams = cJSON_GetObjectItem(cjson_data, "params");
    if(cparams)
    {
        str = cJSON_Print(cJSON_GetObjectItem(cparams, "turn_on"));
        if(str != RT_NULL)
        {
            ctl_status = atoi(str);
            //进行开机操作
            //rt_kprintf("turn on %s\r\n", str);
            if(ctl_status == 1)
                rt_sem_release(computer_on_sem);
            cJSON_free(str);
        }
        str = cJSON_Print(cJSON_GetObjectItem(cparams, "page"));
        if(str != RT_NULL)
        {
            page = atoi(str);
            rt_kprintf("page %s\r\n", str);
            rt_mb_send(page_mail, page);
            cJSON_free(str);
        }
    }
    cJSON_Delete(cjson_data);
}

static void mqtt_sub_default_callback(MQTTClient *c, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
               msg_data->topicName->lenstring.len,
               msg_data->topicName->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(MQTTClient *c)
{
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(MQTTClient *c)
{
    static unsigned char is_first_time_online = 0;
    unsigned char str[40];
    LOG_D("inter mqtt_online_callback!");
    is_online = 1;

    if(is_first_time_online == 0)
    {
        rt_sprintf(str, "{\"method\":\"report\",\"params\":{\"page\":0}}");
        paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, str);
        is_first_time_online = 1;
    }
}

static void mqtt_offline_callback(MQTTClient *c)
{
    LOG_D("inter mqtt_offline_callback!");
    is_online = 0;
}

int mqtt_start(char *user_name, char *passwd)
{
    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[20] = { 0 };

    if (is_started)
    {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = MQTT_URI;

        /* generate the random client ID */
        rt_snprintf(cid, sizeof(cid), "rtthread%d", rt_tick_get());
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.keepAliveInterval = 30;
        client.condata.cleansession = 1;
        client.condata.username.cstring = user_name;
        client.condata.password.cstring = passwd;

        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        client.condata.will.topicName.cstring = MQTT_PUBTOPIC;
        client.condata.will.message.cstring = MQTT_WILLMSG;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = 2048;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        client.messageHandlers[0].topicFilter = rt_strdup(MQTT_SUBTOPIC);
        client.messageHandlers[0].callback = mqtt_sub_callback;
        client.messageHandlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.defaultMessageHandler = mqtt_sub_default_callback;
    }

    /* run mqtt client */
    paho_mqtt_start(&client);
    is_started = 1;

    return 0;
}

static int mqtt_stop(int argc, char **argv)
{
    if (argc != 1)
    {
        rt_kprintf("mqtt_stop    --stop mqtt worker thread and free mqtt client object.\n");
    }

    is_started = 0;

    return paho_mqtt_stop(&client);
}

static int mqtt_publish(int argc, char **argv)
{
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    if (argc == 2)
    {
        paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, argv[1]);
    }
    else if (argc == 3)
    {
        paho_mqtt_publish(&client, QOS1, argv[1], argv[2]);
    }
    else
    {
        rt_kprintf("mqtt_publish <topic> [message]  --mqtt publish message to specified topic.\n");
        return -1;
    }

    return 0;
}

static void mqtt_new_sub_callback(MQTTClient *client, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt new subscribe callback: %.*s %.*s",
               msg_data->topicName->lenstring.len,
               msg_data->topicName->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static int mqtt_subscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_subscribe [topic]  --send an mqtt subscribe packet and wait for suback before returning.\n");
        return -1;
    }

    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_subscribe(&client, QOS1, argv[1], mqtt_new_sub_callback);
}

static int mqtt_unsubscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_unsubscribe [topic]  --send an mqtt unsubscribe packet and wait for suback before returning.\n");
        return -1;
    }

    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_unsubscribe(&client, argv[1]);
}

void status_report_thread(void *param)
{
    unsigned int status;
    unsigned char str[40];
    while(1)
    {
        rt_mb_recv(computer_status_mail, &status, RT_WAITING_FOREVER);
        while(1)
        {
            if (is_started == 0)
            {
                rt_kprintf("mqtt client is not connected.\r\n");
                continue;
            }
            if(is_online == 1)
            {
                rt_sprintf(str, "{\"method\":\"report\",\"params\":{\"status\":%d}}", status);
                if(paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, str) == 0)
                    break;
            }
            rt_thread_mdelay(1000);
        }
    }
}

void create_computer_status_report_thread(void)
{
    rt_thread_t thread;

    thread = rt_thread_create("boot_status_report", status_report_thread, RT_NULL, 1024, 25, 10);
    RT_ASSERT(thread != RT_NULL);
    rt_thread_startup(thread);
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(mqtt_stop, stop mqtt client);
MSH_CMD_EXPORT(mqtt_publish, mqtt publish message to specified topic);
MSH_CMD_EXPORT(mqtt_subscribe,  mqtt subscribe topic);
MSH_CMD_EXPORT(mqtt_unsubscribe, mqtt unsubscribe topic);
#endif /* FINSH_USING_MSH */

