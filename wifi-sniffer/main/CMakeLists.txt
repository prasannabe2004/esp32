idf_component_register(SRCS "wifi_sniffer_main.c" "utils.c"
                    PRIV_REQUIRES spi_flash
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES driver
                    REQUIRES spiffs
                    REQUIRES mbedtls
                    REQUIRES mqtt
                    INCLUDE_DIRS "")
