#ifndef __WIFI_MQTT_CONFIG_H__
#define __WIFI_MQTT_CONFIG_H__
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "globaldefines.h"
#include "gpio_config.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <sys/socket.h>
#include "tcpip_adapter.h"
#include <netdb.h>
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "softap.h"
#include "httpserver.h"
#include "sntp.h"
typedef enum { WIFI_DISCONNECT = 0, WIFI_CONNECTED, WIFI_MQTT_CONNECTED, SMARTCONFIGING, SMARTCONFIG_GETTING } wifi_state_t;

typedef struct jtl_cmda1{
    uint8_t user_temp;
    uint8_t now_temp;
    float outputwater;
    float intputwater;
    uint8_t errorcode;
    uint8_t heater_state1;
    uint8_t heater_state2;
} jtl_cmda1_t;
typedef struct jtl_cmda2{
    float water_yield;
    uint16_t water_volume;
    uint16_t fan_rpm;
} jtl_cmda2_t;
void wifi_init(QueueHandle_t xQueue);
void mqtt_send(const char* data, const char* topic,int retain);
void mqtt_send_cmd(jtl_cmda1_t jtl_a1_data, jtl_cmda2_t jtl_a2_data, uint8_t cmd_type);
const char* get_macAddress(void);
uint8_t get_heaterprior(uint8_t state);
bool set_heaterprior(void);
void judgement(int data_len, const char* event_data, int topic_len, QueueHandle_t xQueue_send);
#endif