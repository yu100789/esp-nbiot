#include "uart_config.h"

static TaskHandle_t nbiot_reg_taskHandle = NULL;
static const char* tag_nbiot = "NBIOT";
static const char* tag_jtl = "JTL";
static const char* tag = "uart";
static bool nb_reg = false;
static void nbiot_send_task(void* parm);
static void nbiot_recv_task(void* parm);
static void nbiot_reg(void* parm);
bool get_nbreg_status(void) { return nb_reg; }
static void nbiot_reg(void* parm)
{
    while (!nb_reg) {
        at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, "AT+CRESET\r", NULL, 6000, 0);
        vTaskDelay(pdMS_TO_TICKS(10000));
        xQueueReset(xQueues.xQueue_nbiot_recv);
        nb_reg = nbiot_register(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv);
    }
    ESP_LOGI(tag_nbiot, "NBIOT MODULE SUCCESSFULLY REG");
    char data[BUF_SIZE];
    char* mqpub[10];
    while (1) {
        if (xQueueReceive(xQueues.xQueue_nbiot_mqtt, data, portMAX_DELAY)) {
            ESP_LOGI(tag_nbiot, "Read to send mqtt mesg : %s", data);
            uint8_t i = 0;
            char* tok_temp = strtok(data, ", ");
            while (tok_temp != NULL) {
                tok_temp = strtok(NULL, ", ");
                mqpub[i] = tok_temp;
                i++;
            }
            uint32_t data_len = strtoul(mqpub[5], NULL, 10) / 2;
            char* data_buf = (char*)calloc(data_len, sizeof(char));
            data_buf[data_len] = 0;
            for (int len = 0; len < data_len; len++) {
                data_buf[len] = convert(mqpub[6][(len * 2) + 1]) << 4 | convert(mqpub[6][(len * 2) + 2]);
            }
            judgement(data_len, data_buf, strlen(mqpub[1]), mqpub[1]);
            free(data_buf);
        }
    }
    nbiot_reg_taskHandle = NULL;
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
            if (strstr((char*)fuck, "+CMQPUB: ") != NULL) {
                ESP_LOGI(tag_nbiot, "recv a MQTT mesg from nbiot");
                if (xQueueSend(xQueues.xQueue_nbiot_mqtt, (void*)fuck, pdMS_TO_TICKS(0)) != pdPASS)
                    ESP_LOGE(tag_nbiot, "ERROR sending mesg to 'xQueue_nbiot_mqtt' maybe full ");
            } else {
                if (xQueueSend(xQueues.xQueue_nbiot_recv, (void*)fuck, pdMS_TO_TICKS(0)) != pdPASS) {
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
void uart_jtl_config(void)
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
}
void uart_nbiot_config(void)
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
}
void nbiotRestart()
{
    nb_reg = false;
    if (nbiot_reg_taskHandle != NULL) {
        vTaskDelete(nbiot_reg_taskHandle);
        nbiot_reg_taskHandle = NULL;
    }
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