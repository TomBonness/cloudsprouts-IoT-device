idf_component_register(SRCS "main.c" "certs.c"
                    INCLUDE_DIRS "."
                    REQUIRES dht cjson
                    PRIV_REQUIRES esp-mqtt esp_wifi nvs_flash)