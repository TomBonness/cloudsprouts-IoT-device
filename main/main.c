#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "dht.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "certs.h"
#include "esp_sntp.h"


#define BROKER_URI "mqtts://a3jxj8x3d0brjo-ats.iot.us-east-1.amazonaws.com"
#define DHT_GPIO 27
#define DHT_TYPE DHT_TYPE_DHT11
#define MQTT_TOPIC "iot/sensors/dht11"
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
static esp_mqtt_client_handle_t client = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("WiFi", "📶 Connected!");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW("WiFi", "❌ Disconnected. Reconnecting...");
        esp_wifi_connect();
    }
}

static void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "Connecting to SSID: %s", WIFI_SSID);

    // Wait until connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .broker.verification.certificate = CERT_ROOT_CA,
        .credentials.authentication = {
            .certificate = CERT_DEVICE,
            .key = CERT_PRIVATE_KEY,
        }
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

static void obtain_time(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org"); // standard public NTP
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2024 - 1900) && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry < retry_count) {
        ESP_LOGI("SNTP", "✅ Time synchronized: %s", asctime(&timeinfo));
    } else {
        ESP_LOGW("SNTP", "❌ Failed to sync time.");
    }
}

void app_main(void) {
    // Init NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_init_sta();
    obtain_time();
    mqtt_app_start();

    printf("🌡️  Starting DHT test\n");

    while (1) {
        float temperature = 0, humidity = 0;
        esp_err_t res = dht_read_float_data(DHT_TYPE, DHT_GPIO, &humidity, &temperature);

        if (res == ESP_OK) {
            printf("✅ Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);

            //time
            time_t now;
            time(&now);
            char timestamp_str[32];
            strftime(timestamp_str, sizeof(timestamp_str), "%FT%TZ", gmtime(&now));

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "deviceID", "esp32-dht11");
            cJSON_AddNumberToObject(root, "temperature", temperature);
            cJSON_AddNumberToObject(root, "humidity", humidity);
            cJSON_AddStringToObject(root, "timestamp", timestamp_str);
            char *json_str = cJSON_PrintUnformatted(root);
            printf("📤 Publishing JSON: %s\n", json_str); // delete later
            cJSON_Delete(root);

            if (client && json_str) {
                esp_mqtt_client_publish(client, MQTT_TOPIC, json_str, 0, 1, 0);
            }

            free(json_str);
        } else {
            printf("⚠️  Failed to read from DHT sensor\n");
        }

        vTaskDelay(pdMS_TO_TICKS(150000));
    }
}
