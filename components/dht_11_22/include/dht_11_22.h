/*
 * Simple library for DHT11/22 sensor
 *
 * Arnaud Pecoraro 2017
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#ifndef __DHT_11_22_h
#define __DHT_11_22_h

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <time.h>
#include <sys/time.h>

#define TIMEOUT_ERROR -111
#define CHECKSUM_ERROR -222
#define SUCCESS 1

#define MIN_TIME 2

void pulse_init();
int checksum(int *cdata);
int wait_change_level(int level, int time);
int read_data();
float temp_c_to_f(float c);

void set_DHT_pin(int pin);
float get_hum();
float get_tempc();
float get_tempf();

#endif 

