#include "services/timer_sync.h"

static const char* TAG = "TIMERS";

TimerHandle_t picture_timer;

EventGroupHandle_t trigger_status;

static void vTimerCallback(TimerHandle_t xTimer) {
    // Enables image capture every 5 minutes
    ESP_LOGI(TAG, "Timer Triggered: Setting TRIGGER_BIT");
    xEventGroupSetBits(trigger_status, TRIGGER_BIT);
}

void init_sntp_and_timer(void) {
    trigger_status = xEventGroupCreate();

    while (!sync_sntp()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    picture_timer = xTimerCreate(
        "picture_timer",                 // Nome
        pdMS_TO_TICKS(TIMER_PERIOD_MS),  // Período em ticks
        pdTRUE,                          // Auto-reload (periódico)
        (void*)0,                        // ID do timer (não usado)
        vTimerCallback                   // Função a ser chamada
    );

    // Inicia o timer
    if (picture_timer != NULL) {
        xTimerStart(picture_timer, 0);
    }
}

void set_timezone(const char* timezone_string) {
    if (setenv("TZ", timezone_string, 1) != 0) {  // Set timezone to UTC-3 without daylight saving
        printf("Error setting TZ environment variable\n");
        return;
    }
    tzset();  // Update C library with the new timezone
}

bool sync_sntp(void) {
    time_t    now      = 0;
    struct tm timeinfo = {0};
    char      strftime_buf[64];

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    while (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGI(TAG, "Sincronizando SNTP");
        time(&now);
        set_timezone("BRT3");  // Set the timezone to UTC-3 (São Paulo)
        localtime_r(&now, &timeinfo);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Relógio sincronizado.");

    return true;
}