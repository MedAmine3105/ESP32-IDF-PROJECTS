idf_component_register(SRCS "main.c"
                       REQUIRES esp_wifi
                       REQUIRES esp_driver_gpio
                       REQUIRES nvs_flash
                       INCLUDE_DIRS ".")
