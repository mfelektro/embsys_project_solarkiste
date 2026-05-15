idf_component_register(SRCS "main.c" "hardware.c"
                    PRIV_REQUIRES esp_driver_gpio esp_driver_i2c
                    INCLUDE_DIRS "")
