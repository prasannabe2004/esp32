idf_component_register(SRCS "ble_server.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS ""
                    REQUIRES bt
                    REQUIRES nvs_flash)
