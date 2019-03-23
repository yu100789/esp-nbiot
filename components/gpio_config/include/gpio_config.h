
#ifndef __GPIO_CONFIG_H__
#define __GPIO_CONFIG_H__

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "globaldefines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GPIO_OUTPUT_IO_0 WIFI_LED
#define GPIO_INPUT_IO_0 18
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0))
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0))
void gpio_pin_init(void* parm);
typedef struct room {
    int status;
    uint32_t sum;
    int id;
} room_t;
room_t get_room_status(void);
EventGroupHandle_t get_gpio_event(void);
QueueHandle_t room_status_queue(void);
#endif