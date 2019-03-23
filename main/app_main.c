#include "app_main.h"

void app_main()
{
    xTaskCreate(gpio_pin_init, "gpio_init", 1024 * 10, NULL, 0, NULL); // had been customized
    // wifi_init();
    xTaskCreate(wifi_state_event, "wifi_state", 1024, NULL, 0, NULL);
    uart_nbiot_config();
    ESP_LOGI(tag, "NBIOT MODULE VERSION : SIM%dE", NBIOT_VERSION);
    ESP_LOGI(tag, "Factory MAC address: %s", get_macAddress());
    ESP_LOGI(tag, "free heap : %d Bytes", esp_get_free_heap_size());
    char fuck[BUF_SIZE];
    EventBits_t nbiot_event, gpio_event;
    room_t room_info;
    // uint32_t val = 0;
    while (1) {
        room_info = get_room_status();
        nbiot_event = xEventGroupWaitBits(get_nbreg_event(), BIT0, false, false, portMAX_DELAY);
        if (nbiot_event & BIT0) {
            gpio_event = xEventGroupWaitBits(get_gpio_event(), BIT0, true, false, portMAX_DELAY);
            if (gpio_event & BIT0) {
                // room_info.sum
                sprintf(fuck, "{\"status\":%d,\"sum\":%d}", room_info.status, room_info.sum);
                ESP_LOGI(tag, "%s", fuck);
                char* ascii = (char*)calloc(BUF_SIZE, sizeof(char));
                convert_ascii(ascii, fuck);
                //                                 qos, retained, dup
                sprintf(fuck, "AT+CMQPUB= 0, \"%s\", 1, 0, 0, %d, \"%s\"\r", DeviceTopic(), strlen(ascii), ascii);
                if (!at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, fuck, "OK", 8000, 0, 0))
                    if (!nbiot_MqttRegeister(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv))
                        nbiotRestart();
                memset((void*)ascii, '\0', BUF_SIZE);
                if (strlen(getToken()) < 2) {
                    ESP_LOGI(tag, "Try to request New Token");
                    convert_ascii(ascii, "get");
                    sprintf(fuck, "AT+CMQPUB= 0, \"%s\", 1, 0, 0, %d, \"%s\"\r", RequestTokenTopic(1), strlen(ascii), ascii);
                    if (!at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, fuck, "OK", 5000, 0, 0))
                        if (!nbiot_MqttRegeister(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv))
                            nbiotRestart();
                }
                free(ascii);
            }
        }
        // val++;
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(tag, "free memory : %d Bytes", esp_get_free_heap_size());
    }
}