#ifndef __ADC_CONTROLLER_H__
#define __ADC_CONTROLLER_H__

#include <esp_log.h>
#include <math.h>
#include <stdio.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* GPIO 1: ADC 1, channel 0 */
#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0

#define ADC_ATTEN    ADC_ATTEN_DB_0             // 0 - 950 mV
#define ADC_BITWIDTH SOC_ADC_DIGI_MAX_BITWIDTH  // 12 bit ADC

#define SAMPLES_COUNT 100

void adc_init(void);

float get_battery(void);

#endif  // __ADC_CONTROLLER_H__