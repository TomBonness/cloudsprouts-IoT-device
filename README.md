# 🌡️ ESP32 + DHT11 + AWS IoT 🌐

This project uses an ESP32 to read temperature and humidity data from a DHT11 sensor, formats the readings as JSON, and sends them to AWS IoT Core via MQTT. It's built with ESP-IDF.

---

## 🗂️ Project Structure

```
📁 components/
  ┣ dht/                 → DHT sensor driver
  ┣ cjson/               → Embedded cJSON for JSON formatting
  ┣ mqtt/                → MQTT client library used to publish data to AWS IoT Core.
  ┗ esp_idf_lib_helpers/ → ESP-IDF compatibility helpers

📁 main/
  ┣ main.c               → Application logic
  ┣ certs.h              → TLS + Wi‑Fi declarations
  ┗ idf_component.yml    → Component dependency manifest

🛠️  CMakeLists.txt       → Project build file  
📦  dependencies.lock    → Locked ESP-IDF version (v5.2.0)  
```

---

## 🚀 How it Works

1. Connects to Wi‑Fi using values from `certs.c`
2. Syncs time using SNTP
3. Connects securely to AWS IoT Core using TLS
4. Reads data from a DHT11 sensor (GPIO 27)
5. Formats sensor values as JSON
6. Publishes to MQTT topic `iot/sensors/dht11`

All logic is in `main/main.c`.

---

## 🛠️ Building the Project

Requires [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) and a set `IDF_PATH`.

```bash
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py flash monitor
```

Before building, add a `certs.c` file in `main/` with your:

- `WIFI_SSID`, `WIFI_PASS`
- `CERT_ROOT_CA`, `CERT_DEVICE`, `CERT_PRIVATE_KEY`

This file is ignored by Git to keep credentials private.

