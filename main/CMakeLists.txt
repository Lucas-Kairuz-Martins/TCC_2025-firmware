file(GLOB app_sources "*.c")

file(GLOB controllers_sources "controllers/*.c")

file(GLOB services_sources "services/*.c")

set(app_include_dirs "services/include" "controllers/include")

idf_component_register(SRCS ${app_sources} ${controllers_sources} ${services_sources}
                    INCLUDE_DIRS ${app_include_dirs}
                    PRIV_REQUIRES
                        nvs_flash
                        esp_psram
                        esp_event
                        esp_http_client
                        esp_wifi
                        mbedtls
                        esp_netif_stack
                        esp_adc)