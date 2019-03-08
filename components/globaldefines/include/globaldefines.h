#ifndef __GLOBALDEFINES_H__
#define __GLOBALDEFINES_H__
/*
=======================================================
                    NEED CUSTOMIZE HERE
=======================================================
*/
#define smarconfig_topic "/esp32/smartconfig"
#define control_state "api/v1/devices/3/control"
#define connect_status "/connect/3/status"
#define heater_power "/connect/3/geyser"
#define general_rules "api/v1/devices/3/rules/#"
#define rules_topic_temperature "api/v1/devices/3/rules/temperature"
#define rules_topic_water "api/v1/devices/3/rules/water"
#define pub_topic_temperature "api/v1/devices/3/logs/temperature"
#define pub_topic_water "api/v1/devices/3/logs/water"
#define auth_token_key "zX9a2bTLZ-JUMJxHDcux"

#define MQTT_HOST_IP "34.217.228.207"
#define MQTT_HOST_PORT 1883
#define WIFI_BAND WIFI_BW_HT20
#define ROOM_ID 4

#define ENABLE_JTL_UART 0

#if ENABLE_JTL_UART
#define WIFI_LED 23
#else
#define WIFI_LED 2
#endif

#define ENABLE_NBIOT 1
#if ENABLE_NBIOT
#define NBIOT_VERSION 7020
#endif

#endif
