#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht11.h"

#define DHT11_GPIO GPIO_NUM_27

void app_main(void) {
    printf("🚀 Starting app_main\n");

    dht11_init(DHT11_GPIO);
    printf("✅ DHT11 initialized on GPIO 26\n");

    vTaskDelay(pdMS_TO_TICKS(1000));  // wait 1 second before first read

    while (1) {
        printf("🔄 Reading from DHT11...\n");
        dht11_reading_t result = dht11_read();

        if (result.temperature != -1 && result.humidity != -1) {
            printf("🌡️  Temp: %d°C  💧 Humidity: %d%%\n", result.temperature, result.humidity);
        } else {
            printf("⚠️  Failed to read from DHT11\n");
        }

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}