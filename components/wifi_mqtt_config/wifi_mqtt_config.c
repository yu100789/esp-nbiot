#include "wifi_mqtt_config.h"

static const char* TAG = "wifi";

esp_mqtt_client_handle_t user_client = 0;
wifi_config_t* wifi_config;
static wifi_state_t wifistate = 0;
static char *gotip, mac_buf[6];
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static uint8_t cmd_b8[16] = { 0x55, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xB8, 0xAA, 0xAA, 0xAA, 0xAA, 0x80, 0x00, 0xAA, 0xAA };
static uint8_t prior_heater = 0;
static int json_temp = 0;
static int s_retry_num = 0;
static TaskHandle_t wificount_handle = NULL;
static TaskHandle_t smartconfig_example_task_handle = NULL;
static TaskHandle_t control_countHandle = NULL;
static QueueHandle_t xQueue_send;

static void mqtt_app_start(void);
static void smartconfig_example_task(void* parm);
static void wificount(void* parm);
static void control_count(void* parm);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    // your_context_t *context = event->context;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        wifistate = WIFI_MQTT_CONNECTED;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_publish(event->client, connect_status, "connected", 9, 0, 1);
        vTaskDelay(100);
        esp_mqtt_client_subscribe(event->client, "/esp32/smartconfig", 0);
        vTaskDelay(100);
        esp_mqtt_client_publish(
            user_client, control_state, "{\"control\":\"0\",\"survive\":\"0\"}", strlen("{\"control\":\"0\",\"survive\":\"0\"}"), 0, 1);
        vTaskDelay(100);
        esp_mqtt_client_subscribe(event->client, control_state, 1);
        vTaskDelay(100);
        esp_mqtt_client_subscribe(event->client, rules_topic_temperature, 1);
        vTaskDelay(100);
        esp_mqtt_client_subscribe(event->client, rules_topic_water, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        wifistate = WIFI_DISCONNECT;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        esp_wifi_disconnect();
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_TOPIC\n %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "MQTT_EVENT_DATA\n %.*s", event->data_len, event->data);
        judgement(event->data_len, event->data, event->topic_len, xQueue_send);
        break;
    case MQTT_EVENT_ERROR:
        wifistate = WIFI_DISCONNECT;
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        esp_wifi_disconnect();
        break;
    }
    return ESP_OK;
}
void mqtt_send(const char* data, const char* topic,int retain)
{
    if (wifistate == WIFI_MQTT_CONNECTED) {
        ESP_LOGI(TAG, "SEND CONTENT : %s", data);
        esp_mqtt_client_publish(user_client, topic, data, strlen(data), 0, retain);
    } else {
        ESP_LOGE(TAG, "WIFI NOT CONNECT %s", data);
    }
}
void mqtt_send_cmd(jtl_cmda1_t jtl_a1_data, jtl_cmda2_t jtl_a2_data, uint8_t cmd_type)
{
    if (wifistate == WIFI_MQTT_CONNECTED) {
        char* json = (char*)calloc(300, sizeof(char));
        tcpip_adapter_ip_info_t ipInfo;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
        gotip = ip4addr_ntoa(&ipInfo.ip);
        if (cmd_type == 0xB1) {
            sprintf(json,
                "{\"source_ip\":\"%s\",\"auth_token\":\"%s\",\"module_serial\":\"%02x%02x%02x%02x%02x%02x\",\"user_temperature\":\"%d\","
                "\"input_temperature\":\"%.1f\","
                "\"output_temperature\":\"%.1f\",\"error_code\":\"%x\",\"status\":\"%x\"}",
                gotip, auth_token_key, mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5], jtl_a1_data.user_temp,
                jtl_a1_data.intputwater, jtl_a1_data.outputwater, jtl_a1_data.errorcode, jtl_a1_data.heater_state1);
            esp_mqtt_client_publish(user_client, pub_topic_temperature, json, strlen(json), 0, 1);
            ESP_LOGI(TAG, "PUBLISHED : %s", json);
            if (set_heaterprior()) {
                sprintf(json, "{\"user\":\"1\",\"temperature\":\"%d\"}", jtl_a1_data.user_temp);
                esp_mqtt_client_publish(user_client, rules_topic_temperature, json, strlen(json), 0, 0);
                ESP_LOGI(TAG, "PUBLISHED : %s", json);
            } else {
                ESP_LOGE(TAG, "NO PRIOR");
            }
        } else if (cmd_type == 0xB2) {
            sprintf(json,
                "{\"source_ip\":\"%s\",\"auth_token\":\"%s\",\"module_serial\":\"%02x%02x%02x%02x%02x%02x\",\"water_yield\":\"%.1f\",\"water_"
                "volume\":\"%d\",\"fan_rpm\":"
                "\"%d\"}",
                gotip, auth_token_key, mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5], jtl_a2_data.water_yield,
                jtl_a2_data.water_volume, jtl_a2_data.fan_rpm);
            // sprintf("%d",jtl_a2_data.user_temp);
            esp_mqtt_client_publish(user_client, pub_topic_water, json, strlen(json), 0, 1);
            ESP_LOGI(TAG, "PUBLISHED : %s", json);
        }
        free(json);
    } else {
        ESP_LOGE(TAG, "WIFI NOT CONNECT");
    }
}
bool set_heaterprior(void)
{
    if (prior_heater == 0) {
        // esp_mqtt_client_publish(
        //     user_client, control_state, "{\"control\":\"1\",\"survive\":\"600\"}", strlen("{\"control\":\"1\",\"survive\":\"600\"}"), 0, 0);
        // prior_heater = 1;
        // if (control_countHandle == NULL)
        //     xTaskCreate(control_count, "control_count", 1024 * 3, (void*)600, 0, &control_countHandle);
        return true;
    } else if (prior_heater == 1) {
        return true;
    }
    return false;
}
uint8_t get_heaterprior(uint8_t state)
{
    if (prior_heater == 0) {
        state = state | 0x00;
    } else if (prior_heater == 1) {
        state = state | 0x10;
    } else if (prior_heater == 2) {
        state = state | 0x08;
    } else {
        ESP_LOGE(TAG, "prior error %d", prior_heater);
    }
    return state;
}
static void control_count(void* parm)
{
    ESP_LOGI(TAG, "CONTROL CONUNT TASK START, CONTENT:\n  %d SECONDS LEFT", (int)parm);
    int survive = (int)parm;
    int i = 0;
    if (survive >= 0) {
        i = survive;
    } else {
        ESP_LOGE(TAG, "CONTROL CONUNT TASK ERROR %d", i);
        goto FAIL;
    }
    while (i >= 0) {
        ESP_LOGI(TAG, "CONTROL SURVIVE: %ds REMAIN", i--);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "CONTROL SURVIVE OVER");
    prior_heater = 0;
    esp_mqtt_client_publish(
            user_client, control_state, "{\"control\":\"0\",\"survive\":\"0\"}", strlen("{\"control\":\"0\",\"survive\":\"0\"}"), 0, 1);
    if(ENABLE_JTL_UART)
        xQueueSend(xQueue_send, (void*)cmd_b8, pdMS_TO_TICKS(0));
FAIL:
    control_countHandle = NULL;
    vTaskDelete(NULL);
}
void judgement(int data_len, const char* event_data, int topic_len, QueueHandle_t xQueue_send)
{
    cJSON* json_data = cJSON_Parse(event_data);
    if (topic_len == strlen(smarconfig_topic)) {
        if (smartconfig_example_task_handle == NULL) {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 1024 * 4, NULL, 6, &smartconfig_example_task_handle);
        }
    } else if (topic_len == strlen(control_state)) {
        ESP_LOGI(TAG, "current prior:%d", prior_heater);
        int control = atoi(cJSON_GetObjectItem(json_data, "control")->valuestring);
        int survive = atoi(cJSON_GetObjectItem(json_data, "survive")->valuestring);
        if (control != 0) {
            if (control_countHandle == NULL) {
                prior_heater = control;
                ESP_LOGI(TAG, "prior change to:%d", prior_heater);
                xTaskCreate(control_count, "control_count", 1024 * 3, (void*)survive, 0, &control_countHandle);
            } else {
                ESP_LOGE(TAG, "CONRTROL COUNT TASK HAD BEEN CREATED");
            }
        }else if(control == 0 && survive == 0){
            if (control_countHandle != NULL) {
                vTaskDelete(control_countHandle);
                control_countHandle = NULL;
                prior_heater = 0;
                ESP_LOGI(TAG, "prior change to:%d", prior_heater);
            }
        }
    } else if (topic_len == strlen(rules_topic_temperature)) {
        int user = atoi(cJSON_GetObjectItem(json_data, "user")->valuestring);
        json_temp = atoi(cJSON_GetObjectItem(json_data, "temperature")->valuestring);
        cmd_b8[8] = (uint8_t)json_temp;
        ESP_LOGI(TAG, "user: %d", user);
        if (user == 2) {
            ESP_LOGI(TAG, "current prior:%d", prior_heater);
            if(ENABLE_JTL_UART)
                xQueueSend(xQueue_send, (void*)cmd_b8, pdMS_TO_TICKS(0));
        } else {
            ESP_LOGI(TAG, "user hasn't been auth");
        }
    } else {
        static uint8_t cmd_b9[16] = { 0x55, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xB9, 0xAA, 0xAA, 0xAA, 0xAA, 0x80, 0x00, 0xAA, 0xAA };
        unsigned long water = strtoul(event_data, NULL, 10);
        cmd_b9[8] = water >> 8;
        cmd_b9[9] = water;
        if (prior_heater != 1) {
            ESP_LOGI(TAG, "current prior:%d", prior_heater);
            cmd_b9[12] |= 0x04;
            if(ENABLE_JTL_UART)
                xQueueSend(xQueue_send, (void*)cmd_b9, pdMS_TO_TICKS(0));
        } else {
            ESP_LOGI(TAG, "no prior,cancel cmd sending");
        }
    }
    cJSON_Delete(json_data);
    ESP_LOGI(TAG, "free memory : %d Bytes", esp_get_free_heap_size());
}
static void wificount(void* parm)
{
    wifi_bandwidth_t bw;
    ESP_ERROR_CHECK(esp_wifi_get_bandwidth(ESP_IF_WIFI_STA, &bw));
    ESP_LOGI(TAG, "WIFI BANDWIDTH : %d", bw * 20);
    esp_wifi_connect();
    vTaskDelay(pdMS_TO_TICKS(15000));
    // esp_wifi_stop();
    // esp_wifi_deinit();
    // ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    // wifi_init_softap();
    if (smartconfig_example_task_handle != NULL) {
        vTaskDelete(smartconfig_example_task_handle);
        smartconfig_example_task_handle = NULL;
    }
    xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 1024 * 4, NULL, 6, &smartconfig_example_task_handle);
    wificount_handle = NULL;
    vTaskDelete(NULL);
}
static esp_err_t wifi_event_handler(void* ctx, system_event_t* event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_AP_START:
        start_tests();
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_START:
        if(wificount_handle == NULL)
            xTaskCreate(wificount, "wificount", 1024*8, 0, 0, &wificount_handle);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        wifistate = WIFI_CONNECTED;
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        mqtt_app_start();
        if(ENABLE_HTTP_SERVER)
            start_tests();  //httpserver
        xTaskCreate(sntp_initialize,"sntp_initialize", 1024*8, NULL, 0, NULL);
        if (wificount_handle != NULL) {
            vTaskDelete(wificount_handle);
            wificount_handle = NULL;
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        wifistate = WIFI_DISCONNECT;
        ESP_LOGE(TAG, "DISCONNECT REASON : %d", event->event_info.disconnected.reason);
        if (s_retry_num < 10) {
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
        }else{
            if(wificount_handle == NULL)
                xTaskCreate(wificount, "wificount", 1024*8, 0, 0, &wificount_handle);
        }
        ESP_LOGI(TAG,"connect to the AP fail\n");
        if (event->event_info.disconnected.reason >= 8 && event->event_info.disconnected.reason != 201)
            esp_restart();
        if (user_client != 0)
            esp_mqtt_client_stop(user_client);
        break;
    default:
        break;
    }
    return ESP_OK;
}
static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = { 
        .host = MQTT_HOST_IP,
        .port = MQTT_HOST_PORT,
        .keepalive = 180,
        .disable_auto_reconnect = false,
        .event_handle = mqtt_event_handler,
        .task_prio = 10,
        .lwt_topic = connect_status,
        .lwt_msg = "offline",
        .lwt_qos = 1,
        .lwt_retain = 1,
        .lwt_msg_len = 7
    };
    user_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(user_client);
}
static void sc_callback(smartconfig_status_t status, void* pdata)
{
    switch (status) {
    case SC_STATUS_WAIT:
        ESP_LOGI(TAG, "SC_STATUS_WAIT");
        break;
    case SC_STATUS_FIND_CHANNEL:
        wifistate = SMARTCONFIGING; //   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        wifistate = SMARTCONFIG_GETTING;
        ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
        break;
    case SC_STATUS_LINK:
        ESP_LOGI(TAG, "SC_STATUS_LINK");
        wifi_config = (wifi_config_t*)pdata;
        ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BAND));
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SC_STATUS_LINK_OVER:
        ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
        if (pdata != NULL) {
            uint8_t phone_ip[4] = { 0 };
            memcpy(phone_ip, (uint8_t*)pdata, 4);
            ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
        }
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
        break;
    default:
        break;
    }
}

static void smartconfig_example_task(void* parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK(esp_smartconfig_stop());
    // ESP_ERROR_CHECK(esp_esptouch_set_timeout(60));
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    // ESP_ERROR_CHECK(esp_smartconfig_fast_mode(false));
    ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if (uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            smartconfig_example_task_handle = NULL;
            vTaskDelete(NULL);
        }
    }
}
static void wifi_state_event(void* parm)
{
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_OUTPUT_IO_0, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(GPIO_OUTPUT_IO_0, GPIO_PULLUP_ONLY));
    while (1) {
        switch (wifistate) {
        case WIFI_DISCONNECT:
            gpio_set_level(GPIO_OUTPUT_IO_0, 0);
            break;
        case WIFI_CONNECTED:
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            break;
        case WIFI_MQTT_CONNECTED:
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            vTaskDelay(pdMS_TO_TICKS(250));
            break;
        case SMARTCONFIGING:
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case SMARTCONFIG_GETTING:
            vTaskDelay(pdMS_TO_TICKS(50));
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
            break;
        default:
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }
        gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    }
}
void wifi_init(QueueHandle_t xQueue)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    xQueue_send = xQueue;
    nvs_flash_init();
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_read_mac((uint8_t*)mac_buf, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "factory set MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BAND));
    xTaskCreate(wifi_state_event, "wifi_state", 1024, NULL, 0, NULL);
}
const char* get_macAddress(void)
{
    static char mac[13];
    esp_read_mac((uint8_t*)mac_buf, ESP_MAC_WIFI_STA);
    sprintf(mac,"%02x%02x%02x%02x%02x%02x", mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);
    return mac;
}