/*
 * task_control - controle do aquecedor (rele) por histerese (liga-desliga).
 *
 * Objetivo: segurar a temperatura na faixa do aquecedor.
 *   - aquecendo e T >= HEATER_OFF_ABOVE_C (29 C) -> DESLIGA e deixa esfriar
 *   - parado    e T <= HEATER_ON_BELOW_C  (28 C) -> LIGA e aquece de novo
 * Entre 28 e 29 mantem o estado atual (a "banda morta" evita ligar/desligar
 * toda hora).
 *
 * Seguranca: se a leitura de temperatura falhar, desliga o aquecedor.
 */
#include "tasks.h"
#include "shared_state.h"
#include "app_config.h"
#include "rele.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "control";

void task_control(void *arg)
{
    (void)arg;
    bool heating = false;

    rele_set(false);
    shared_state_set_heater(false);

    for (;;) {
        sensor_data_t d;
        shared_state_get(&d);

        if (d.sensor_ok) {
            if (heating) {
                if (d.temperature >= HEATER_OFF_ABOVE_C) {
                    heating = false;
                    ESP_LOGI(TAG, "T=%.2f >= %.2f -> DESLIGA aquecedor",
                             d.temperature, (float)HEATER_OFF_ABOVE_C);
                }
            } else {
                if (d.temperature <= HEATER_ON_BELOW_C) {
                    heating = true;
                    ESP_LOGI(TAG, "T=%.2f <= %.2f -> LIGA aquecedor",
                             d.temperature, (float)HEATER_ON_BELOW_C);
                }
            }
        } else if (heating) {
            heating = false;   /* sem leitura confiavel: desliga por seguranca */
            ESP_LOGW(TAG, "sensor sem leitura -> DESLIGA aquecedor");
        }

        rele_set(heating);
        shared_state_set_heater(heating);

        vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIODO_MS));
    }
}
