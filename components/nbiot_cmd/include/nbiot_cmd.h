#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "globaldefines.h"
#include "uart_config.h"
#include "wifi_mqtt_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
bool nbiot_register(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv);
bool at_cmd(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv, const char* cmd, const char* cmd_response, uint16_t cmd_timeout, bool resend);
uint8_t CSQ(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv);