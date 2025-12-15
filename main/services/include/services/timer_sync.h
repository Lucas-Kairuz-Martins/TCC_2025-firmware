#ifndef __TIMER_SYNC_H__
#define __TIMER_SYNC_H__

#include <esp_log.h>

#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define TRIGGER_BIT (1 << 0)

#define TIMER_PERIOD_MS (3 * 60 * 1000)  // 3 minutes
// #define TIMER_PERIOD_MS (60 * 1000)  // 1 minute

extern TimerHandle_t picture_timer;

extern EventGroupHandle_t trigger_status;

void set_timezone(const char* timezone_string);

void init_sntp_and_timer(void);

bool sync_sntp(void);

#endif  // __TIMER_SYNC_H__