/*
 * task_dht11 - le a umidade do DHT11 periodicamente e publica em shared_state.
 * Escreve apenas o campo de umidade (nao mexe em temperatura/pressao, que vem
 * do BMP280 na task_sensor). Sem logica de negocio: so le e publica.
 */
#include "tasks.h"
#include "shared_state.h"
#include "app_config.h"
#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "dht11";

void task_dht11(void *arg)
{
    (void)arg;

    /* DHT11 precisa de ~1 s apos energizar antes da 1a leitura confiavel. */
    vTaskDelay(pdMS_TO_TICKS(1500));

    for (;;) {
        dht11_reading_t r;
        esp_err_t e = ESP_FAIL;

        /* DHT11 e ruidoso; tenta ate 3x antes de marcar como falha. */
        for (int i = 0; i < 3 && e != ESP_OK; i++) {
            e = dht11_read(&r);
            if (e != ESP_OK)
                vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (e == ESP_OK) {
            shared_state_set_humidity(r.humidity, true);
            ESP_LOGI(TAG, "H=%.0f %%RH (T_dht=%.0f C)", r.humidity, r.temperature);
        } else {
            shared_state_set_humidity(0.0f, false);
            ESP_LOGW(TAG, "leitura falhou (%s)", esp_err_to_name(e));
        }

        vTaskDelay(pdMS_TO_TICKS(DHT11_PERIODO_MS));
    }
}
