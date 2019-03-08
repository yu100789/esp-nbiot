#ifndef __WIFI_MQTT_CONFIG_H__
#define __WIFI_MQTT_CONFIG_H__
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
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tcpip_adapter.h"
#include <netdb.h>
#include <sys/socket.h>
typedef enum { WIFI_DISCONNECT = 0, WIFI_CONNECTED, WIFI_MQTT_CONNECTED, SMARTCONFIGING, SMARTCONFIG_GETTING } wifi_state_t;
void wifi_init(void);
void mqtt_send(const char* data, const char* topic, int retain);
const char* get_macAddress(void);
const char* get_ip(void);
int get_wifiState(void);
esp_mqtt_client_handle_t getClientHandle(void);
#endif