
#include "nbiot_cmd.h"
static const char* TAG = "NBIOT";
void nbiot_reset(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    printf("nbiot restart\n");
    xQueueSend(xQueue_send, &"AT+CFUN=0\r", portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(15000));
    xQueueSend(xQueue_send, &"AT+CFUN=1\r", portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(15000));
}
bool nbiot_register(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    static char fuck[100];
    while (!at_cmd(xQueue_send, xQueue_recv, "AT\r", "OK", 10000, 1))
        ;
    if (!at_cmd(xQueue_send, xQueue_recv, "ATE0\r", "OK", 6000, 1))
        goto cmd_fail;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+IPR=115200\r", "OK", 6000, 1))
        goto cmd_fail;
    while (CSQ(xQueue_send, xQueue_recv) < 5)
        ;
    while (!at_cmd(xQueue_send, xQueue_recv, "AT+CGREG?\r", "+CGREG: 0,1", 6000, 1))
        ;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CGREG?\r", "+CGREG: 0,1", 6000, 1))
    //     goto cmd_fail;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+CGCONTRDP\r", NULL, 15000, 1))
        goto cmd_fail;
    if (!at_cmd(xQueue_send, xQueue_recv, "AT+CMQDISCON=0\r", NULL, 15000, 1))
        goto cmd_fail;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CMQNEW=\"54.95.211.233\",\"1883\",12000,1000\r", "OK", 6000, 0))
    sprintf(fuck, "AT+CMQNEW=\"%s\",\"%d\",12000,1000\r", MQTT_HOST_IP, MQTT_HOST_PORT);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 10000, 0))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQCON=0,3,\"nbiot_%s\",6000,0,0\r", get_macAddress());
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQPUB=0,\"%s\",0,1,0,18,\"636F6E6E6563746564\"\r", connect_status);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, NULL, 5000, 0))
        goto cmd_fail;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CTZU=1\r", "OK", 5000, 0))
    //     goto cmd_fail;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CLTS=1\r", "OK", 5000, 0))
    //     goto cmd_fail;
    // if (!at_cmd(xQueue_send, xQueue_recv, "AT+CSNTPSTART=\"time.stdtime.gov.tw\"\r", "OK", 10000, 1))
    //     goto cmd_fail;
#if ENABLE_JTL_UART
    sprintf(fuck, "AT+CMQSUB=0,\"%s\",0\r", rules_topic_temperature);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQSUB=0,\"%s\",0\r", rules_topic_water);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0))
        goto cmd_fail;
    sprintf(fuck, "AT+CMQSUB=0,\"%s\",0\r", control_state);
    if (!at_cmd(xQueue_send, xQueue_recv, fuck, "OK", 5000, 0))
        goto cmd_fail;
#endif
    ESP_LOGI(TAG, "nbiot reg complete");
    return true;
cmd_fail:
    ESP_LOGE(TAG, "nbiot need reset");
    nbiot_reset(xQueue_send, xQueue_recv);
    return false;
}
bool at_cmd(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv, const char* cmd, const char* cmd_response, uint16_t cmd_timeout, bool resend)
{
    uint8_t i = 0;
    char fuck[BUF_SIZE];
    xQueueSend(xQueue_send, (void*)cmd, portMAX_DELAY);
    while (1) {
        if (xQueueReceive(xQueue_recv, fuck, pdMS_TO_TICKS(cmd_timeout))) {
            if (cmd_response == NULL) {
                i = 0;
                break;
            }
            const char* data = strstr(fuck, cmd_response);
            if (data) {
                ESP_LOGI(TAG, "recv OK from nbiot");
                i = 0;
                break;
            } else {
                ESP_LOGE(TAG, "recv ERROR from nbiot");
            }
        } else {
            ESP_LOGE(TAG, "recv from nbiot timeout");
        }
        if (i > 5) {
            i = 0;
            return false;
        }
        i++;
        vTaskDelay(pdMS_TO_TICKS(2000));
        if (resend) {
            xQueueSend(xQueue_send, (void*)cmd, portMAX_DELAY);
        }
    }
    i = 0;
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
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
const char* GPS(QueueHandle_t xQueue_send, QueueHandle_t xQueue_recv)
{
    char fuck[BUF_SIZE];
    uint8_t i = 0;
    while (1) {
        if (i == 3) {
            return "unknow";
        }
        xQueueSend(xQueue_send, (void*)&"AT+CGNSINF\r", portMAX_DELAY);
        if (xQueueReceive(xQueue_recv, fuck, pdMS_TO_TICKS(5000))) {
            char* data = strstr(fuck, "CGNSINF");
            if (data) {
                ESP_LOGI(TAG, "nbiot send OK");
                return data;
            } else {
                ESP_LOGE(TAG, "nbiot send ERROR");
            }
        } else {
            ESP_LOGE(TAG, "recv from nbiot timeout\n");
        }
        i++;
    }
}