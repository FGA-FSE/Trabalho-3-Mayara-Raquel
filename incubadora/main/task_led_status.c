/*
 * task_led_status - traduz o estado atual em cor no LED RGB.
 * Consome shared_state; usa o componente rgb_led (PWM).
 */
#include "tasks.h"
#include "shared_state.h"
#include "rgb_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_led_status(void *arg)
{
    (void)arg;
    bool blink = false;

    for (;;) {
        sensor_data_t d;
        shared_state_get(&d);
        incub_status_t st = compute_status(&d);

        switch (st) {
            /* Faixa segura (27 a 32 C = ideal +/- banda) -> VERDE. */
            case ST_OK:
            case ST_WARNING: rgb_led_set(0,   255, 0);   break;  /* verde    */
            /* Fora da faixa (< 27 ou > 32 C) -> VERMELHO. */
            case ST_ALARM:   rgb_led_set(255, 0,   0);   break;  /* vermelho */
            /* Estados que nao dependem da temperatura mantem o azul. */
            case ST_ERROR:   blink = !blink;                     /* falha de leitura */
                             rgb_led_set(0, 0, blink ? 255 : 0); break;  /* azul piscando */
            default:         rgb_led_set(0, 0, 255);     break;  /* azul fixo (init) */
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
