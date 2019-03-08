#include "nbiot_mqtt.h"

static const char* tag = "NBIOT_MQTT";
static char Token[100];
void judgement(int data_len, const char* data, int topic_len, const char* topic)
{
    ESP_LOGI(tag, "Topic :%s, Data :%s", topic, data);
    cJSON* json_data = cJSON_Parse(data);
    if (strstr(topic, RequestTokenTopic(0)) != NULL) {
        strcpy(Token, data);
        ESP_LOGI(tag, "Get Token : %s", Token);
    }
    cJSON_Delete(json_data);
    ESP_LOGI(tag, "free memory : %d Bytes", esp_get_free_heap_size());
}
const char* getToken(void)
{
    if (strlen(Token) > 1)
        return Token;
    else 
        return "0";
}
/*
    0   :post

    any :get
*/
const char* RequestTokenTopic(int type)
{
    static char token_topic[100];
    if (type)
        sprintf(token_topic, "/get/%s/token", get_macAddress());
    else
        sprintf(token_topic, "/post/%s/token", get_macAddress());
    return token_topic;
}
const char* DeviceTopic()
{
    static char token_topic[100];
    sprintf(token_topic, "/device/%s/token/%s/data", get_macAddress(), getToken());
    return token_topic;
}