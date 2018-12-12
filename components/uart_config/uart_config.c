#include "uart_config.h"

static jtl_cmda1_t jtl_a1_data;
static jtl_cmda2_t jtl_a2_data;
static TaskHandle_t jtl_recv_taskhandle = NULL, jtl_send_taskhandle = NULL, nbiot_reg_taskHandle = NULL;
static const char* tag_nbiot = "NBIOT";
static const char* tag_jtl = "JTL";
static const char* tag = "uart";
static bool nb_reg = false;
static char timestamp[100];
static void nbiot_send_task(void* parm);
static void nbiot_recv_task(void* parm);
static void nbiot_reg(void* parm);
static void jtl_recv_task(void* parm);
static void jtl_send_task(void* parm);
bool get_nbreg_status(void) { return nb_reg; }
static void jtl_send_task(void* parm)
{
    uint8_t* data = (uint8_t*)malloc(BUF_SIZE);
    while (1) {
        if (xQueueReceive(xQueues.xQueue_jtl_send, data, portMAX_DELAY)) {
            ESP_LOGI(tag_jtl, "GET CMD :");
            ESP_LOG_BUFFER_HEX(tag_jtl, data, 16);
            if (data[7] == 0xA1 || data[7] == 0xA2) {
                data[7] += 0x10;
                data[12] = get_heaterprior(data[12]);
                data[14] = crc_high_first(data, 14);
                uart_write_bytes(UART_NUM_1, (const char*)data, 16);
                ESP_LOGI(tag_jtl, "send cmdACK");
                ESP_LOG_BUFFER_HEX(tag, data, 16);
                uint8_t* buf = (uint8_t*)calloc(BUF_SIZE + 1, sizeof(uint8_t));
                memcpy(buf, data, 17);
                int i = 0;
                while (i < 3) {
                    if (xQueueReceive(xQueues.xQueue_jtl_recv, data, pdMS_TO_TICKS(500))) {
                        if (data[15] == 0xAA) {
                            if (data[14] == crc_high_first(data, 14)) {
                                if (data[7] == 0xC1) {
                                    if (buf[7] == 0xB1) {
                                        jtl_a1_data.user_temp = buf[8];
                                        jtl_a1_data.now_temp = jtl_a1_data.user_temp;
                                        jtl_a1_data.outputwater = buf[9];
                                        jtl_a1_data.outputwater /= 2;
                                        jtl_a1_data.intputwater = buf[10];
                                        jtl_a1_data.intputwater /= 2;
                                        jtl_a1_data.errorcode = buf[11];
                                        jtl_a1_data.heater_state1 = buf[12];
                                        jtl_a1_data.heater_state2 = buf[13];
                                        mqtt_send_cmd(jtl_a1_data, jtl_a2_data, buf[7]);
                                    } else if (buf[7] == 0xB2) {
                                        jtl_a2_data.water_yield = buf[8];
                                        jtl_a2_data.water_yield /= 10;
                                        jtl_a2_data.water_volume = buf[9] << 8 | buf[10];
                                        jtl_a2_data.fan_rpm = buf[11] << 8 | buf[12];
                                        mqtt_send_cmd(jtl_a1_data, jtl_a2_data, buf[7]);
                                    }
                                    ESP_LOGI(tag_jtl, "A1 or A2 CMD COMPLETE");
                                    break;
                                } else {
                                    ESP_LOGE(tag_jtl, "CMD ERROR");
                                }
                            } else {
                                ESP_LOGE(tag_jtl, "CRC ERROR 2 : %x", (char)crc_high_first(data, 14));
                            }
                        } else {
                            ESP_LOGE(tag_jtl, "NOT A CMD");
                        }
                    } else {
                        uart_write_bytes(UART_NUM_1, (const char*)buf, 16);
                        ESP_LOGI(tag_jtl, "resend cmd");
                        ESP_LOG_BUFFER_HEX(tag_jtl, buf, 16);
                        i++;
                    }
                }
                free(buf);
            } else if (data[7] == 0xB8 || data[7] == 0xB9) {
                data[12] = get_heaterprior(data[12]);
                data[14] = crc_high_first(data, 14);
                uart_write_bytes(UART_NUM_1, (const char*)data, 16);
                ESP_LOGI(tag_jtl, "send cmdACK");
                ESP_LOG_BUFFER_HEX(tag_jtl, data, 16);
                uint8_t* buf = (uint8_t*)calloc(BUF_SIZE + 1, sizeof(uint8_t));
                memcpy(buf, data, 17);
                int i = 0;
                while (i < 3) {
                    if (xQueueReceive(xQueues.xQueue_jtl_recv, data, pdMS_TO_TICKS(500))) {
                        if (data[15] == 0xAA) {
                            if (data[14] == crc_high_first(data, 14)) {
                                if (data[7] == 0xA8 || data[7] == 0xA9) {
                                    if (data[7] == 0xA8) {
                                        data[7] = 0xC2;
                                        data[14] = crc_high_first(data, 14);
                                        uart_write_bytes(UART_NUM_1, (const char*)data, 16);
                                        ESP_LOGI(tag_jtl, "B8 CMD COMPLETE");
                                        break;
                                    } else if (data[7] == 0xA9) {
                                        data[7] = 0xC2;
                                        data[14] = crc_high_first(data, 14);
                                        uart_write_bytes(UART_NUM_1, (const char*)data, 16);
                                        ESP_LOGI(tag_jtl, "B9 CMD COMPLETE");
                                        break;
                                    }
                                } else {
                                    ESP_LOGE(tag_jtl, "CMD ERROR");
                                }
                            } else {
                                ESP_LOGE(tag_jtl, "CRC ERROR 3 : %x", (char)crc_high_first(data, 14));
                            }
                        } else {
                            ESP_LOGE(tag_jtl, "NOT A CMD");
                        }
                    } else {
                        uart_write_bytes(UART_NUM_1, (const char*)buf, 16);
                        ESP_LOGI(tag_jtl, "resend cmd");
                        ESP_LOG_BUFFER_HEX(tag_jtl, buf, 16);
                        i++;
                    }
                }
                free(buf);
            }
            ESP_LOGI(tag, "free memory : %d Bytes", esp_get_free_heap_size());
        }
    }
    free(data);
    vTaskDelete(NULL);
}
static void jtl_recv_task(void* parm)
{
    // Configure a temporary buffer for the incoming data
    uint8_t* data = (uint8_t*)malloc(BUF_SIZE + 1);
    while (1) {
        const int len = uart_read_bytes(UART_NUM_1, (uint8_t*)data, BUF_SIZE, 20 / portTICK_RATE_MS);
        if (len > 0) {
            data[len] = 0;
            // ESP_LOGI(tag_jtl, "JTL_UART Read %d bytes: '%s'", len, data);
            ESP_LOGI(tag_jtl, "JTL_UART Read %d bytes", len);
            if (data[15] == 0xAA) {
                if (data[14] == crc_high_first(data, 14)) {
                    if (data[7] == 0xA1 || data[7] == 0xA2) {
                        xQueueSend(xQueues.xQueue_jtl_send, (void*)data, pdMS_TO_TICKS(500));
                    } else if (data[7] == 0xA8 || data[7] == 0xA9 || data[7] == 0xC1) {
                        xQueueSend(xQueues.xQueue_jtl_recv, (void*)data, pdMS_TO_TICKS(500));
                    } else {
                        ESP_LOGE(tag_jtl, "CMD ERROR");
                    }
                } else {
                    ESP_LOGE(tag_jtl, "CRC ERROR 1 : %x", (char)crc_high_first(data, 14));
                }
            } else {
                ESP_LOGE(tag_jtl, "NOT A CMD");
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
}
static void nbiot_reg(void* parm)
{
    while (!nb_reg) {
        nb_reg = nbiot_register(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(tag_nbiot, "NBIOT MODULE SUCCESSFULLY REG");
    char* data = (char*)malloc(BUF_SIZE);
    char* mqpub[10];
#if ENABLE_JTL_UART
    while (1) {
        if (xQueueReceive(xQueues.xQueue_nbiot_mqtt, data, portMAX_DELAY)) {
            ESP_LOGI(tag_nbiot, "Read to send mqtt mesg : %s", data);
            static uint8_t i = 0;
            char* test = strtok(data, ", ");
            while (test != NULL) {
                test = strtok(NULL, ", ");
                mqpub[i] = test;
                i++;
            }
            i = 0;
            uint32_t data_len = strtoul(mqpub[5], NULL, 10) / 2;
            char* data_buf = (char*)calloc(data_len, sizeof(char));
            data_buf[data_len] = 0;
            for (int len = 0; len < data_len; len++) {
                data_buf[len] = convert(mqpub[6][(len * 2) + 1]) << 4 | convert(mqpub[6][(len * 2) + 2]);
            }
            if (strstr(mqpub[1], control_state) != NULL) {
                ESP_LOGI(tag_nbiot, " CONTROL topic : %s, DATA : %s, data_len : %d", mqpub[1], data_buf, data_len);
                judgement(data_len, data_buf, strlen(control_state), xQueues.xQueue_jtl_send);
            } else if (strstr(mqpub[1], rules_topic_temperature) != NULL) {
                ESP_LOGI(tag_nbiot, "TEMPERATURE topic : %s, DATA : %s, data_len : %d", mqpub[1], data_buf, data_len);
                judgement(data_len, data_buf, strlen(rules_topic_temperature), xQueues.xQueue_jtl_send);
            } else if (strstr(mqpub[1], rules_topic_water) != NULL) {
                ESP_LOGI(tag_nbiot, "WATER topic : %s, DATA : %s, data_len : %d", mqpub[1], data_buf, data_len);
                judgement(data_len, data_buf, strlen(rules_topic_water), xQueues.xQueue_jtl_send);
            } else {
                ESP_LOGE(tag_nbiot, "find nothing match to topic");
            }
            free(data_buf);
        }
    }
#endif
    free(data);
    vTaskDelete(NULL);
}
static void nbiot_recv_task(void* parm)
{
    uint8_t fuck[BUF_SIZE + 1];
    while (1) {
        const int len = uart_read_bytes(UART_NUM_2, fuck, BUF_SIZE, 20 / portTICK_RATE_MS);
        if (len > 0) {
            fuck[len] = 0;
            ESP_LOGI(tag_nbiot, "NBIOT_UART Read %d bytes:'%s'", len, fuck);
            if (strstr((char*)fuck, "+CMQPUB") != NULL) {
                ESP_LOGI(tag_nbiot, "recv a MQTT mesg from nbiot");
                if (xQueueSend(xQueues.xQueue_nbiot_mqtt, (void*)fuck, pdMS_TO_TICKS(0)) != pdPASS)
                    ESP_LOGE(tag_nbiot, "ERROR sending mesg to 'xQueue_nbiot_mqtt' maybe full ");
            } else if (strstr((char*)fuck, "+CSNTP") != NULL) {
                ESP_LOGI(tag_nbiot, "recv a SNTP mesg from nbiot");
                memcpy(timestamp, fuck, len);
            } else {
                if (xQueueSend(xQueues.xQueue_nbiot_recv, (void*)fuck, pdMS_TO_TICKS(100)) != pdPASS) {
                    ESP_LOGE(tag_nbiot, "ERROR sending mesg to 'xQueue_nbiot_recv' maybe full ");
                    xQueueReset(xQueues.xQueue_nbiot_recv);
                }
            }
        }
    }
    vTaskDelete(NULL);
}
static void nbiot_send_task(void* parm)
{
    // Configure a temporary buffer for the incoming data
    char fuck[BUF_SIZE + 1];
    while (1) {
        if (xQueueReceive(xQueues.xQueue_nbiot_send, fuck, portMAX_DELAY)) {
            ESP_LOGI(tag_nbiot, "%s", fuck);
            uart_write_bytes(UART_NUM_2, fuck, strlen(fuck));
        }
    }
    vTaskDelete(NULL);
}
int uart_jtl_config(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = { .baud_rate = 38400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD, RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    xQueues.xQueue_jtl_send = xQueueCreate(1, 100);
    xQueues.xQueue_jtl_recv = xQueueCreate(1, 100);
    xTaskCreate(jtl_recv_task, "jtl_recv_task", 1024 * 4, NULL, 4, &jtl_recv_taskhandle);
    xTaskCreate(jtl_send_task, "jtl_send_task", 1024 * 4, NULL, 5, &jtl_send_taskhandle);
    return 1;
}
int uart_nbiot_config(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = { .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
    xQueues.xQueue_nbiot_send = xQueueCreate(1, BUF_SIZE);
    xQueues.xQueue_nbiot_recv = xQueueCreate(5, BUF_SIZE);
    xQueues.xQueue_nbiot_mqtt = xQueueCreate(10, BUF_SIZE);
    ESP_LOGI(tag, "waiting for nbiot bootup");
    vTaskDelay(pdMS_TO_TICKS(20000));
    xTaskCreate(nbiot_send_task, "nbiot_send_task", 1024 * 4, NULL, 2, NULL);
    xTaskCreate(nbiot_recv_task, "nbiot_recv_task", 1024 * 8, NULL, 3, NULL);
    xTaskCreate(nbiot_reg, "nbiot_reg", 1024 * 4, NULL, 0, &nbiot_reg_taskHandle);
    return 1;
}
void nbiotRestart()
{
    nb_reg = false;
    if (nbiot_reg_taskHandle != NULL) {
        vTaskDelete(nbiot_reg_taskHandle);
        nbiot_reg_taskHandle = NULL;
    }
    xQueueReset(xQueues.xQueue_nbiot_recv);
    xTaskCreate(nbiot_reg, "nbiot_reg", 1024 * 4, NULL, 0, &nbiot_reg_taskHandle);
}
uint8_t crc_high_first(uint8_t* ptr, int len)
{
    uint8_t i;
    uint8_t crc = 0x00;
    while (len--) {
        crc ^= *ptr++;
        for (i = 8; i > 0; --i) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x13;
            else
                crc = (crc << 1);
        }
    }
    return (crc);
}
unsigned char convert(unsigned char ch)
{
    unsigned char value;
    switch (ch) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        value = ch + 10 - 'A';
        break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        value = ch + 10 - 'a';
        break;
    default:
        value = ch - '0';
    }
    return value;
}
void get_nbiotTime(char* timedata)
{
    char buf[50],*temp;
    if (!at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, "AT+CSNTPSTART=\"time.stdtime.gov.tw\"\r", "OK", 10000, 1))
        nbiotRestart();
    vTaskDelay(pdMS_TO_TICKS(1000));
    memcpy(buf, timestamp, strlen(timestamp));
    temp = strtok(buf, " ");
    temp = strtok(NULL, "\r");
    ESP_LOGI(tag, "%s", temp);
    memcpy(timedata, temp, strlen(temp));
}
void convert_ascii(char* ascii, const char* raw)
{
    int i = 0;
    char buf[2];
    while (raw[i] != '\0') {
        sprintf(buf, "%x", raw[i]);
        strcat(ascii, buf);
        i++;
    }
    // memcpy(ascii, asciiData, strlen(asciiData));
    // free(asciiData);
}