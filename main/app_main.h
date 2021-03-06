#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "globaldefines.h"
#include "gpio_config.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "nbiot_cmd.h"
#include "adc_config.h"
#include "uart_config.h"
#include "wifi_mqtt_config.h"
#include "cJSON.h"
static const char* tag = "system_app";
#endif
