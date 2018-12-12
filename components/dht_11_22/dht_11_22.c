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
 *
 *    Short note on data coding scheme:
 *
 *    Low level of ~50us followed by high level
 *    high level duration determines the symbol:
 *         - high level of ~26us is a 0 whereas ~70us is 1
 *
 *    It is therefore possible to deduce the symbol by comparing
 *    high/low level duration.
 *
 *                  26us           70us
 *        --+     +------+     +---------+
 *          |     |      |     |         |
 *          |     |      |     |         |
 *          |     |      |     |         |
 *          +-----+      +-----+         +---
 *
 *                   "0"           "1"
 *
 *    See DHT22 datasheet for more in-depth informations.
 *
 */

#include "dht_11_22.h"

int DHT_GPIO = 18;
int timeout;
int time_diff = -MIN_TIME;
int data[2] = {0};

static const char *TAG = "DHT sensor";

void pulse_init()
{
    gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(DHT_GPIO, 0);
    ets_delay_us(20000);
    gpio_set_level(DHT_GPIO, 1);
    ets_delay_us(40);

    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
}

int checksum(int *cdata)
{
    return (cdata[4] == ((cdata[0] + cdata[1] + cdata[2] + cdata[3]) & 0xFF));
}

void set_DHT_pin(int pin){
    DHT_GPIO = pin;
}

int wait_change_level(int level, int time)
{
    int cpt = 0;
    while (gpio_get_level(DHT_GPIO) == level) {
       if (cpt > time) {
           timeout = 1;
       }
       ++cpt;
       ets_delay_us(1);
    }
    return cpt;
}

int read_data()
{
    int i, val[80] = {0}, err, bytes[5] = {0};
    char out[40] = {0};
    timeout = 0;

    struct timeval now;
    gettimeofday(&now, NULL);

    if(now.tv_sec - time_diff < MIN_TIME)
        return SUCCESS;
    else{
        gpio_pad_select_gpio(DHT_GPIO);

        portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

        portENTER_CRITICAL(&my_spinlock); // timing critical start
        {
            // Init sequence MCU side: pulse and wait
            pulse_init();
            ets_delay_us(10);

            // DHT22 sending init sequence(high/low/high)
            wait_change_level(1, 40);
            wait_change_level(0, 80);
            wait_change_level(1, 80);

            // And now the reading adventure begins
            for (i = 0; i < 80; i += 2)
            {
                val[i] = wait_change_level(0, 80);
                val[i+1] = wait_change_level(1, 80);
            }
        }
        portEXIT_CRITICAL(&my_spinlock); // timing critical end

        for (i = 2; i < 80; i+=2)
            out[i/2] = (val[i] < val[i+1]) ? 1 : 0;

        for (i = 0; i < 40; ++i) {
            bytes[i/8] <<= 1;
            bytes[i/8] |= out[i];
        }

        if(timeout)
        {
            ESP_LOGE(TAG, "Timeout error\n");
            err = TIMEOUT_ERROR;
        }
        else if(!checksum(bytes))
        {
            ESP_LOGE(TAG, "Checksum error\n");
            err = CHECKSUM_ERROR;
        }
        else
        {
            for (i = 0; i < 2; ++i)
                data[i] = (bytes[2*i] << 8) + bytes[2*i+1];

            gettimeofday(&now, NULL);
            time_diff = now.tv_sec;
            err = SUCCESS;
        }

        return err;
    }
}


float get_hum()
{
    int success = read_data();
    return success ? (float) data[0]/10 : success;
}

float get_tempc()
{
    int success = read_data();
    return success ? (float) data[1]/10 : success;
}

float temp_c_to_f(float c)
{
    return c * 9/5 +32;
}

float get_tempf()
{
    int success = read_data();
    return success ? temp_c_to_f((float) data[1]/10): success;
}
