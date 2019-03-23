
#include "nbiot_cmd.h"
static const char* TAG = "NBIOT";
bool nbiot_register(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    char fuck[100];
    while (!at_cmd(xQueue_send, xQueue_recv, "AT\r", "OK", 10000, 1,0))
        ;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+IPR=115200\r", "OK", 6000, 1,0))
        goto cmd_fail;
    while (CSQ(xQueue_send, xQueue_recv) < 5)
        ;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CGREG=2\r", "OK", 6000, 0))
    //     goto cmd_fail;
    // while (!at_cmd(xQueue_send, xQueue_recv, "AT+CGREG?\r", "+CGREG: 2,1", 6000, 1))
    //     ;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+CGREG?\r", "+CGREG: 0,1", 6000, 0,0))
        goto cmd_fail;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+CGCONTRDP\r", NULL, 15000, 1,0))
        goto cmd_fail;
    if (!nbiot_MqttRegeister(xQueue_send, xQueue_recv))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQSUB=0,\"%s\",1\r", RequestTokenTopic(0));
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0,0))
        goto cmd_fail;
    if (strlen(getToken()) < 2) {
        char* ascii = (char*)calloc(1024, sizeof(char));
        convert_ascii(ascii, "get");
        sprintf(fuck, "AT+CMQPUB= 0, \"%s\", 1, 0, 0, %d, \"%s\"\r", RequestTokenTopic(1), strlen(ascii), ascii);
        free(ascii);
        if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0,0))
            goto cmd_fail;
    }
    ESP_LOGI(TAG, "nbiot reg complete");
    vTaskDelay(pdMS_TO_TICKS(5000));
    return true;
cmd_fail:
    ESP_LOGE(TAG, "nbiot need reset");
    return false;
}
bool at_cmd(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv, const char* cmd, const char* cmd_response, uint16_t cmd_timeout, bool resend,
    bool error_break)
{
    xQueueReset(xQueue_recv);
    uint8_t i = 0;
    char fuck[BUF_SIZE];
    xQueueSend(xQueue_send, (void*)cmd, portMAX_DELAY);
    while (1) {
        if (xQueueReceive(xQueue_recv, fuck, pdMS_TO_TICKS(cmd_timeout))) {
            if (cmd_response == NULL) {
                break;
            }
            const char* data = strstr(fuck, cmd_response);
            if (data) {
                ESP_LOGI(TAG, "recv OK from nbiot");
                break;
            } else {
                ESP_LOGE(TAG, "recv ERROR from nbiot");
                if (error_break) {
                    vTaskDelay(pdMS_TO_TICKS(cmd_timeout));
                    return false;
                }
            }
        } else {
            ESP_LOGE(TAG, "recv from nbiot timeout");
        }
        if (i++ > 4) {
            return false;
        }
        // vTaskDelay(pdMS_TO_TICKS(cmd_timeout));
        else if (resend) {
            xQueueSend(xQueue_send, (void*)cmd, portMAX_DELAY);
        }
    }
    return true;
}

uint8_t CSQ(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    char fuck[BUF_SIZE];
    uint8_t i = 0;
    while (1) {
        if (i > 5) {
            return 0;
        }
        xQueueSend(xQueue_send, (void*)&"AT+CSQ\r", portMAX_DELAY);
        if (xQueueReceive(xQueue_recv, fuck, pdMS_TO_TICKS(5000))) {
            const char* data = strstr(fuck, "+CSQ: ");
            if (data) {
                ESP_LOGI(TAG, "nbiot send OK");
                uint8_t csq = (data[6] - 0x30) * 10;
                if (csq != 0)
                    csq += (data[7] - 0x30);
                ESP_LOGI(TAG, "csq :%d", csq);
                if (csq > 5 && csq != 99) {
                    return csq;
                } else {
                    ESP_LOGE(TAG, "csq below 5 or equal 99");
                }
            } else {
                ESP_LOGE(TAG, "nbiot send ERROR");
            }
        } else {
            ESP_LOGE(TAG, "recv from nbiot timeout\n");
        }
        i++;
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
bool nbiot_MqttRegeister(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    char fuck[100];
    at_cmd(xQueue_send, xQueue_recv, "AT+CMQDISCON=0\r", NULL, 15000, 1,0);
    vTaskDelay(pdMS_TO_TICKS(5000));
    sprintf(fuck, "AT+CMQNEW=\"%s\",\"%d\",12000,1000\r", MQTT_HOST_IP, MQTT_HOST_PORT);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 15000, 0,0))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQCON=0,4,\"nbiot%s\",6000,0,0,\"%s\",\"%s\"\r", get_macAddress(), "iot", "iotTECH");
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0,0))
        goto cmd_fail;
    return true;
cmd_fail:
    return false;
}