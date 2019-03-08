#include "app_main.h"

void app_main()
{
    xTaskCreate(gpio_pin_init, "gpio_init", 1024 * 10, NULL, 0, NULL);
    ESP_LOGI(tag, "gpio init ok");
    // #if ENABLE_JTL_UART
    //     if (uart_jtl_config())
    //         ESP_LOGI(tag, "jtl uart init ok");
    // #endif
    uart_nbiot_config();
    ESP_LOGI(tag, "NBIOT MODULE VERSION : SIM%dE", NBIOT_VERSION);
    ESP_LOGI(tag, "Factory MAC address: %s", get_macAddress());
    ESP_LOGI(tag, "free heap : %d Bytes", esp_get_free_heap_size());
    char fuck[BUF_SIZE];
    room_t room_info;
    unsigned long val = 0;
    int error_time = 0;
    while (1) {
        room_info = get_room_status();
        if (get_nbreg_status()) {
            // room_info.sum
            sprintf(fuck, "{\"status\":%d,\"sum\":%ld}", room_info.status, val);
            ESP_LOGI(tag, "%s", fuck);
            char* ascii = (char*)calloc(BUF_SIZE, sizeof(char));
            convert_ascii(ascii, fuck);
            //                                qos, retained, dup
            sprintf(fuck, "AT+CMQPUB= 0, \"%s\", 1, 1, 1, %d, \"%s\"\r", DeviceTopic(), strlen(ascii), ascii);
            if (at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, fuck, "OK", 5000, 0))
                error_time = 0;
            else
                error_time++;
            memset((void*)ascii, '\0', BUF_SIZE);
            if (strlen(getToken()) < 2) {
                ESP_LOGI(tag, "Try to request New Token");
                convert_ascii(ascii, "get");
                sprintf(fuck, "AT+CMQPUB= 0, \"%s\", 1, 0, 1, %d, \"%s\"\r", RequestTokenTopic(1), strlen(ascii), ascii);
                if (at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, fuck, "OK", 5000, 0))
                    error_time = 0;
                else
                    error_time++;
            }
            if (error_time > 2) {
                nbiotRestart();
                error_time = 0;
            }
            free(ascii);
        }
        val++;
        gpio_set_level(GPIO_OUTPUT_IO_0, val);
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(tag, "free memory : %d Bytes", esp_get_free_heap_size());
    }
}