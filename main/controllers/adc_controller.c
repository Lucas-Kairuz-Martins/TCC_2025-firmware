#include "controllers/adc_controller.h"

#define LSB (850.0 / (pow(2.0, 12.0)))  // 12 bit ADC with 0dB attenuation (100-950mV)

#define VOLTAGE_DIVIDER_RATIO (110.0 / (470.0 + 110.0))  // R1 = 470k, R2 = 110k

#define mV_to_V 0.001

static const char* TAG = "ADC_controller";

static adc_oneshot_unit_handle_t adc1_handle;

void adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten    = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "ADC iniciado");
}

float get_battery(void) {
    int adc_raw;

    int adc_avg = 0;

    for (int i = 0; i < SAMPLES_COUNT; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));

        adc_avg += adc_raw;
    }

    adc_avg = adc_avg / SAMPLES_COUNT;

    return (mV_to_V * ((float)adc_avg * LSB) / VOLTAGE_DIVIDER_RATIO);
}