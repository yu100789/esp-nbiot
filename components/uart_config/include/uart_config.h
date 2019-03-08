#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "globaldefines.h"
#include "nbiot_cmd.h"
#include "soc/uart_struct.h"
#include "nbiot_mqtt.h"
#include <stdio.h>
#define TXD (GPIO_NUM_4)
#define RXD (GPIO_NUM_5)
#define TXD2 (GPIO_NUM_26)
#define RXD2 (GPIO_NUM_27)
#define BUF_SIZE (1024)
typedef struct xQueue {
    QueueHandle_t xQueue_nbiot_send;
    QueueHandle_t xQueue_nbiot_recv;
    QueueHandle_t xQueue_jtl_send;
    QueueHandle_t xQueue_jtl_recv;
    QueueHandle_t xQueue_nbiot_mqtt;
} xQueue_t;
xQueue_t xQueues;
void uart_jtl_config(void);
void uart_nbiot_config(void);
uint8_t crc_high_first(uint8_t* ptr, int len);
unsigned char convert(unsigned char ch);
void convert_ascii(char* ascii, const char* raw);
bool get_nbreg_status(void);
void nbiotRestart();
void get_nbiotTime(char* timedata);
#endif