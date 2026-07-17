/*
 * task_sensor - le o BME280/BMP280 periodicamente e publica em shared_state.
 * Sem logica de negocio: apenas le e publica o dado bruto.
 */
#include "tasks.h"
#include "shared_state.h"
#include "app_config.h"
#include "bme280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "sensor";

void task_sensor(void *arg)
{
    bme280_dev_t *dev = (bme280_dev_t *)arg;

    for (;;) {
        bme280_reading_t r;

        /* So temperatura e pressao aqui; a umidade vem do DHT11 (task_dht11). */
        if (dev && bme280_read(dev, &r) == ESP_OK) {
            shared_state_set_env(r.temperature, r.pressure, true);
            ESP_LOGI(TAG, "T=%.2f C  P=%.1f hPa", r.temperature, r.pressure);
        } else {
            shared_state_set_env(0.0f, 0.0f, false);
            ESP_LOGW(TAG, "leitura falhou");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIODO_MS));
    }
}
