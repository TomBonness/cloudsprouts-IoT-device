#include "dht11.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rom/ets_sys.h"

#define TAG "DHT11"
#define MAX_WAIT 100  // 100us timeout

static gpio_num_t dht11_pin;

void dht11_init(gpio_num_t pin) {
    dht11_pin = pin;
    gpio_set_direction(dht11_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht11_pin, 1);
}

static bool wait_for_level(int expected_level, int timeout_us) {
    int t = 0;
    while (gpio_get_level(dht11_pin) != expected_level) {
        if (t++ > timeout_us) {
            ESP_LOGW(TAG, "Timeout waiting for level %d", expected_level);
            return false;
        }
        ets_delay_us(1);
    }
    return true;
}

dht11_reading_t dht11_read() {
    dht11_reading_t result = { .temperature = -1, .humidity = -1 };
    uint8_t data[5] = {0};

    // Start signal
    gpio_set_direction(dht11_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht11_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10)); // ≥18ms

    gpio_set_level(dht11_pin, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(dht11_pin, GPIO_MODE_INPUT);

    // Wait for DHT11 response
    if (!wait_for_level(0, 200)) return result;
    if (!wait_for_level(1, 200)) return result;

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        if (!wait_for_level(0, 50)) return result;

        int start = esp_timer_get_time();
        if (!wait_for_level(1, 70)) return result;
        int pulse = esp_timer_get_time() - start;

        data[i / 8] <<= 1;
        if (pulse > 40) {
            data[i / 8] |= 1;
        }
    }

    // Checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        ESP_LOGW(TAG, "Checksum fail: %d vs %d", sum, data[4]);
        return result;
    }

    result.humidity = data[0];
    result.temperature = data[2];
    return result;
}
