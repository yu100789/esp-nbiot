#include "gpio_config.h"

#define ESP_INTR_FLAG_DEFAULT 0
static const char* TAG = "gpio";
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle room_status_handle = NULL;
static TaskHandle_t empty_check_handle = NULL;
static room_t room_info = { false, 0, ROOM_ID };
room_t get_room_status(void) { return room_info; }
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
static void empyt_check(void*arg)
{
    ESP_LOGI(TAG, "empyt check create");
    vTaskDelay(pdMS_TO_TICKS(3600000));
    ESP_LOGE(TAG, "empty room detected");
    room_info.status = false;
    vTaskDelete(NULL);
}
static void gpio_task(void* arg)
{
    ESP_LOGI(TAG, "IR Module Ready");
    uint32_t io_num;
    int lastStatus = 0;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d", io_num, gpio_get_level(io_num));
            if (room_info.status) {
                room_info.status = false;
                ESP_LOGI(TAG, "empyt check delete");
                vTaskDelete(empty_check_handle);
                room_info.sum++;
                goto refresh;
            }
            room_info.status = true;
            xTaskCreate(empyt_check, "empyt_check", 1024, NULL, 0, &empty_check_handle);
        refresh:
            if (lastStatus != room_info.status) {
                ESP_LOGI(TAG, "room status change %d to %d", lastStatus, room_info.status);
                lastStatus = room_info.status;
            }
        }
    }
}
QueueHandle_t room_status_queue(void)
{
    if (room_status_handle != NULL)
        return room_status_handle;
    else
        return NULL;
}
void gpio_pin_init(void* parm)
{
    room_status_handle = xQueueCreate(100, sizeof(int));
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_0, 1);
    //  type of interrupt
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // setting pull-up mode
    io_conf.pull_up_en = 0;
    // setting pull-down mode
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(60000));
    // change gpio intrrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task, "gpio_task", 1024 * 4, NULL, 0, NULL);
    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*)GPIO_INPUT_IO_0);
    vTaskDelete(NULL);
}