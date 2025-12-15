#ifndef __WEB_SERVICE_H__
#define __WEB_SERVICE_H__

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mbedtls/base64.h"
#include "services/timer_sync.h"

// --- Globais do Wi-Fi ---
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_init(void);

esp_err_t _http_event_handler(esp_http_client_event_t* evt);

#endif  // __WEB_SERVICE_H__