#include "app_main.h"

static void MyTask1(void* parm)
{
    static char fuck[BUF_SIZE];
    static room_t room_info;
    uint8_t val = 0;
    while (1) {
        room_info = get_room_status();
        // get_time(timestamp);
        // sprintf(
        //     fuck, "{\"status\":\"%d\",\"sum\":\"%d\",\"id\":\"%d\",\"timestamp\":\"%s\"}", room_info.status, room_info.sum, room_info.id,
        //     timestamp);
        // mqtt_send(fuck, "flower_room", 1);
        if (get_nbreg_status()) {
            sprintf(fuck, "{\"status\":\"%d\",\"sum\":\"%d\",\"id\":\"%d\"}", room_info.status, room_info.sum, room_info.id);
            char* ascii = (char*)calloc(BUF_SIZE, sizeof(char));
            convert_ascii(ascii, fuck);
            ESP_LOGI(tag, "%s", fuck);
            sprintf(fuck, "AT+CMQPUB=0,\"flower_room\",0,1,1,%d,\"%s\"\r", strlen(ascii), ascii);
            if (!at_cmd(xQueues.xQueue_nbiot_send, xQueues.xQueue_nbiot_recv, fuck, "OK", 8000, 1))
                nbiotRestart();
            free(ascii);
        }
        val = ~val;
        gpio_set_level(GPIO_OUTPUT_IO_0, val);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}
static void MyTask2(void* parm)
{
    static char fuck[200], timestamp[100];
    uint32_t adc_pin32, adc_pin34, adc_pin35, adc_pin36;
    int temp, hum;
    setDHTPin(GPIO_INPUT_IO_0);
    ESP_ERROR_CHECK(gpio_set_direction(14, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(14, GPIO_PULLUP_ONLY));
    while (1) {
        get_time(timestamp);
        temp = getTemp();
        vTaskDelay(pdMS_TO_TICKS(3000));
        hum = getHumidity();
        if (temp < 0)
            errorHandle(temp);
        adc_pin32 = get_adc_value(32, false); // set false to transfer to Voltage
        adc_pin34 = get_adc_value(34, false);
        adc_pin35 = get_adc_value(35, false);
        adc_pin36 = get_adc_value(36, false);
        ESP_LOGI(tag, "Temperature:%d\tHumidity: %d\tadc_32:%dmV\tadc_34:%dmV\tadc_35:%dmV\tadc_36:%dmV", temp, hum, adc_pin32, adc_pin34, adc_pin35,
            adc_pin36);
        sprintf(fuck,
            "{\"Temperature\":\"%d\",\"Humidity\":\"%d\",\"CO\":\"%d\",\"Gas\":\"%d\",\"CH4\":\"%d\",\"C4H10\":\"%d\",\"Alarm\":\"%d\",\"timestamp\":"
            "\"%s\"}",
            temp, hum, adc_pin32, adc_pin34, adc_pin35, adc_pin36, gpio_get_level(14), timestamp);
        mqtt_send(fuck, "sensors", 1);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
    vTaskDelete(NULL);
}
void app_main()
{
#if ENABLE_GPIO
    xTaskCreate(gpio_pin_init, "gpio_init", 1024 * 10, NULL, 0, NULL);
    ESP_LOGI(tag, "gpio init ok");
    xTaskCreate(MyTask1, "MyTask1", 1024 * 10, NULL, 0, NULL);
#endif
#if ENABLE_JTL_UART
    if (uart_jtl_config())
        ESP_LOGI(tag, "jtl uart init ok");
#endif
#if ENABLE_WIFI
    wifi_init(xQueues.xQueue_jtl_send);
#endif
#if ENABLE_NBIOT
    if (uart_nbiot_config()) {
        ESP_LOGI(tag, "NBIOT MODULE VERSION : SIM%dE", NBIOT_VERSION);
        ESP_LOGI(tag, "nbiot uart init ok");
    }
#endif
#if ENABLE_ADC
    xTaskCreate(AdcMainTask, "ADC_TASK", 1024 * 8, NULL, 0, NULL);
    xTaskCreate(MyTask2, "MyTask2", 1024 * 5, NULL, 6, NULL);
#endif
    ESP_LOGI(tag, "Factory MAC address: %s", get_macAddress());
    ESP_LOGI(tag, "Customized MAC address: %s", MAC_ADDRESS);
    ESP_LOGI(tag, "free heap : %d Bytes", esp_get_free_heap_size());
    vTaskDelete(NULL);
}