#ifndef __ADC_CONFIG_H__
#define __ADC_CONFIG_H__
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "globaldefines.h"

#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64 // Multisampling

typedef struct adc_info {
    adc_unit_t unit;
    adc_channel_t channel;
    adc_atten_t atten;
    esp_adc_cal_characteristics_t * chars;
    uint32_t voltage;
    uint32_t raw;
} adc_info_t;

void adc_initConfig(void);
uint32_t get_adc_value(int adc_pin,bool raw_type);
#endif