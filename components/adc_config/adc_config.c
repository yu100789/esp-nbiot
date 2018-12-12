#include "adc_config.h"
static const char* TAG = "adc";
static adc_info_t ADC_PIN36 = { .channel = ADC1_CHANNEL_0, .atten = ADC_ATTEN_11db, .chars = NULL, .voltage = 0, .raw = 0 };
static adc_info_t ADC_PIN35 = { .channel = ADC1_CHANNEL_7, .atten = ADC_ATTEN_11db, .chars = NULL, .voltage = 0, .raw = 0 };
static adc_info_t ADC_PIN34 = { .channel = ADC1_CHANNEL_6, .atten = ADC_ATTEN_11db, .chars = NULL, .voltage = 0, .raw = 0 };
static adc_info_t ADC_PIN32 = { .channel = ADC1_CHANNEL_4, .atten = ADC_ATTEN_11db, .chars = NULL, .voltage = 0, .raw = 0 };
static const adc_unit_t unit = ADC_UNIT_1;

static void check_efuse()
{
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}
static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

void AdcMainTask(void* parm)
{
    adc_power_on();
    // Check if Two Point or Vref are burned into eFuse
    check_efuse();
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_PIN32.channel, ADC_PIN32.atten);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_PIN34.channel, ADC_PIN34.atten);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_PIN35.channel, ADC_PIN35.atten);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_PIN36.channel, ADC_PIN36.atten);

    // Characterize ADC
    ADC_PIN32.chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    ADC_PIN34.chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    ADC_PIN35.chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    ADC_PIN36.chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, ADC_PIN32.atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, ADC_PIN32.chars);
    print_char_val_type(val_type);
    esp_adc_cal_characterize(unit, ADC_PIN34.atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, ADC_PIN34.chars);
    esp_adc_cal_characterize(unit, ADC_PIN35.atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, ADC_PIN35.chars);
    esp_adc_cal_characterize(unit, ADC_PIN36.atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, ADC_PIN36.chars);
    // Continuously sample ADC1
    while (1) {
        // Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            ADC_PIN32.raw += adc1_get_raw(ADC_PIN32.channel);
            ADC_PIN34.raw += adc1_get_raw(ADC_PIN34.channel);
            ADC_PIN35.raw += adc1_get_raw(ADC_PIN35.channel);
            ADC_PIN36.raw += adc1_get_raw(ADC_PIN36.channel);
        }
        ADC_PIN32.raw /= NO_OF_SAMPLES;
        ADC_PIN34.raw /= NO_OF_SAMPLES;
        ADC_PIN35.raw /= NO_OF_SAMPLES;
        ADC_PIN36.raw /= NO_OF_SAMPLES;
        // Convert adc_reading to voltage in mV
        ADC_PIN32.voltage = esp_adc_cal_raw_to_voltage(ADC_PIN32.raw, ADC_PIN32.chars);
        ADC_PIN34.voltage = esp_adc_cal_raw_to_voltage(ADC_PIN34.raw, ADC_PIN34.chars);
        ADC_PIN35.voltage = esp_adc_cal_raw_to_voltage(ADC_PIN35.raw, ADC_PIN35.chars);
        ADC_PIN36.voltage = esp_adc_cal_raw_to_voltage(ADC_PIN36.raw, ADC_PIN36.chars);
        // ESP_LOGI(TAG,"Raw:     %d  \t%d  \t%d  \t%d\n", adc_reading_0, adc_reading_1, adc_reading_2, adc_reading_3);
        // ESP_LOGI(TAG,"Voltage: %dmv\t%dmV\t%dmv\t%dmv\n", ADC_PIN32.voltage, ADC_PIN34.voltage, ADC_PIN35.voltage, ADC_PIN36.voltage);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
uint32_t get_adc_value(int adc_pin, bool raw_type)
{
    switch (adc_pin) {
    case 32:
        if (!raw_type)
            return ADC_PIN32.voltage;
        else
            return ADC_PIN32.raw;
        break;
    case 34:
        if (!raw_type)
            return ADC_PIN34.voltage;
        else
            return ADC_PIN34.raw;
        break;
    case 35:
        if (!raw_type)
            return ADC_PIN35.voltage;
        else
            return ADC_PIN35.raw;
        break;
    case 36:
        if (!raw_type)
            return ADC_PIN36.voltage;
        else
            return ADC_PIN36.raw;
        break;
    default:
        break;
    }
    return 0;
}
