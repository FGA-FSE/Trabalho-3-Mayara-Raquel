/*
 * task_buzzer - alarme sonoro quando a temperatura sai da faixa segura.
 * Apita de forma intermitente enquanto o estado for ST_ALARM (T < 27 ou > 32,
 * usando as faixas ideal +/- banda do shared_state). Fora disso, fica em silencio.
 */
#include "tasks.h"
#include "shared_state.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BEEP_MS   250   /* duracao do apito e da pausa (apito-apito) */

void task_buzzer(void *arg)
{
    (void)arg;

    for (;;) {
        sensor_data_t d;
        shared_state_get(&d);
        incub_status_t st = compute_status(&d);

        if (st == ST_ALARM) {
            buzzer_set(true);
            vTaskDelay(pdMS_TO_TICKS(BEEP_MS));
            buzzer_set(false);
            vTaskDelay(pdMS_TO_TICKS(BEEP_MS));
        } else {
            buzzer_set(false);
            vTaskDelay(pdMS_TO_TICKS(BEEP_MS));
        }
    }
}
