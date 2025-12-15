#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#include "controllers/adc_controller.h"
#include "controllers/cam_controller.h"
#include "esp_camera.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mbedtls/base64.h"
#include "services/timer_sync.h"
#include "services/web_service.h"

static const char* TAG = "MAIN";

/*** AWS Endpoint ***/
#define SEU_API_GATEWAY_URL                                          \
    "https://xguhdujwm8.execute-api.sa-east-1.amazonaws.com/teste1/" \
    "upload-imagem"

// Certificado: Amazon Root CA 1
// Obtido de "https://www.amazontrust.com/repository/AmazonRootCA1.pem"
const char* aws_root_ca_pem =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

void take_and_upload_picture(void) {
    char*                    json_payload = NULL;
    esp_http_client_handle_t client       = NULL;
    unsigned char*           base64_buf   = NULL;
    size_t                   base64_len   = 0;

    picture_data_t pic_data = {0};

    // --- 1. Capturar Imagem ---
    pic_data = take_picture();

    // --- 2. Capturar Bateria ---
    float bateria = get_battery();

    time_t    now;
    char      strftime_buf[64];
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Formato ISO 8601 (ex: "2025-11-03T17:45:00")
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // --- 3. Codificar Imagem para Base64 ---
    // Calcula o tamanho necessário para o buffer Base64
    mbedtls_base64_encode(NULL, 0, &base64_len, pic_data.pic->buf, pic_data.pic->len);
    pic_data.base64_buf = (unsigned char*)malloc(base64_len);
    if (pic_data.base64_buf == NULL) {
        ESP_LOGE(TAG, "Falha ao alocar memória para Base64");
        goto cleanup;
    }
    // Codifica
    if (mbedtls_base64_encode(pic_data.base64_buf, base64_len, &base64_len, pic_data.pic->buf, pic_data.pic->len) !=
        0) {
        ESP_LOGE(TAG, "Falha ao codificar para Base64");
        goto cleanup;
    }

    // --- 4. Montar o JSON ---
    ESP_LOGI(TAG, "Montando JSON...");
    int json_len = asprintf(
        &json_payload, "{\"imagem_base64\": \"%s\", \"bateria\": %f, \"timestamp\": \"%s\"}",
        (const char*)pic_data.base64_buf, bateria, strftime_buf
    );

    if (json_len < 0 || json_payload == NULL) {
        ESP_LOGE(TAG, "Falha ao montar JSON (asprintf)");
        goto cleanup;
    }
    ESP_LOGI(TAG, "Payload JSON criado (Tamanho: %d bytes)", json_len);

    // --- 5. Enviar Requisição HTTPS POST ---
    ESP_LOGI(TAG, "Iniciando cliente HTTPS para %s", SEU_API_GATEWAY_URL);
    esp_http_client_config_t config = {
        .url           = SEU_API_GATEWAY_URL,
        .event_handler = _http_event_handler,
        .cert_pem      = aws_root_ca_pem,  // Anexa o certificado Raiz da Amazon
        .method        = HTTP_METHOD_POST,
        .timeout_ms    = 15000,  // Timeout de 15 segundos
    };
    client = esp_http_client_init(&config);

    // Define o tipo de conteúdo como JSON
    esp_http_client_set_header(client, "Content-Type", "application/json");
    // Define o corpo (body) da requisição
    esp_http_client_set_post_field(client, json_payload, json_len);

    ESP_LOGI(TAG, "Enviando requisição POST...");
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Status do HTTPS POST: %d", status_code);
        if (status_code == 200) {
            ESP_LOGI(TAG, "Upload realizado com SUCESSO!");
        } else {
            ESP_LOGE(TAG, "Upload FALHOU (Servidor respondeu com %d)", status_code);
        }
    } else {
        ESP_LOGE(TAG, "Falha na requisição HTTPS: %s", esp_err_to_name(err));
    }

    // --- 6. Limpeza (Cleanup) ---
cleanup:
    ESP_LOGI(TAG, "Limpando memória...");
    if (client) {
        esp_http_client_cleanup(client);
    }
    if (json_payload)
        free(json_payload);

    free_picture_data(&pic_data);
}

void task(void* pvParameters) {
    static EventBits_t bits;

    while (1) {
        bits = xEventGroupWaitBits(trigger_status, TRIGGER_BIT, pdTRUE, pdFALSE, 0);

        if (bits & TRIGGER_BIT) {
            ESP_LOGI(TAG, "Evento: Capturar Imagem");
            take_and_upload_picture();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    // --- 1. Inicializar NVS (Necessário para o Wi-Fi) ---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // --- 2. Inicializar Câmera e ADC ---
    if (ESP_OK != init_camera()) {
        ESP_LOGE(TAG, "Falha na Câmera! Reiniciando...");
        esp_restart();
    }

    adc_init();

    // --- 3. Conectar ao Wi-Fi ---
    wifi_init();

    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Delay de 2 segundoss

    // --- 4. Sincronizar Relógio ---
    init_sntp_and_timer();

    ESP_LOGI(TAG, "Criando a task...........................");

    static EventBits_t bits;

    take_and_upload_picture();

    xTaskCreatePinnedToCore(
        task, "main_task", 32768, NULL,
        configMAX_PRIORITIES - 1  // Priority
        ,
        NULL,
        1  // core number (0/1)
    );

    vTaskSuspend(NULL);  // Suspend app_main

    // while (1) {
    //     bits = xEventGroupWaitBits(trigger_status, TRIGGER_BIT, pdTRUE, pdFALSE, 0);

    //     if (bits & TRIGGER_BIT) {
    //         ESP_LOGI(TAG, "Evento: Capturar Imagem");
    //         take_and_upload_picture();
    //     }

    //     battery_voltage = get_battery();

    //     ESP_LOGI(TAG, "Nível da bateria (ADC raw): %f", battery_voltage);

    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}