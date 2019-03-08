#ifndef __NBIOT_MQTT_H__
#define __NBIOT_MQTT_H__

#include "uart_config.h"
void judgement(int data_len, const char* data, int topic_len, const char* topic);
const char* getToken(void);
const char* RequestTokenTopic(int type);
const char* DeviceTopic(void);
#endif