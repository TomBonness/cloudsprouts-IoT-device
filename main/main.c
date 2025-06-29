#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"

#define DHT_GPIO 27
#define DHT_TYPE DHT_TYPE_DHT11

void app_main(void) {
    printf("🌡️  Starting DHT test\n");

    while (1) {
        float temperature = 0, humidity = 0;
        esp_err_t res = dht_read_float_data(DHT_TYPE, DHT_GPIO, &humidity, &temperature);

        if (res == ESP_OK) {
            printf("✅ Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
        } else {
            printf("⚠️  Failed to read from DHT sensor\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}