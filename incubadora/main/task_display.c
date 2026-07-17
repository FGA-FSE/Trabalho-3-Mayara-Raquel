/*
 * task_display - desenha a leitura atual no OLED SSD1306.
 * Consome shared_state; nao conversa direto com o sensor.
 */
#include "tasks.h"
#include "shared_state.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *estado_txt(incub_status_t s)
{
    switch (s) {
        case ST_OK:      return "IDEAL";
        case ST_WARNING: return "ALERTA";
        case ST_ALARM:   return "FORA";
        case ST_ERROR:   return "ERRO";
        default:         return "INIT";
    }
}

void task_display(void *arg)
{
    (void)arg;
    for (;;) {
        sensor_data_t d;
        shared_state_get(&d);
        incub_status_t st = compute_status(&d);

        char l_temp[24], l_aux[24];
        if (d.sensor_ok) {
            snprintf(l_temp, sizeof(l_temp), "%.2f~C", d.temperature);
            if (d.has_humidity)
                snprintf(l_aux, sizeof(l_aux), "U:%.0f%% P:%.0f", d.humidity, d.pressure);
            else
                snprintf(l_aux, sizeof(l_aux), "P:%.0f", d.pressure);
        } else {
            snprintf(l_temp, sizeof(l_temp), "-- ~C");
            snprintf(l_aux,  sizeof(l_aux),  "SEM SENSOR");
        }

        ssd1306_clear();
        ssd1306_draw_string(2,  0,  "INCUBADORA", 1);
        ssd1306_draw_string(4,  12, l_temp, 2);          /* temperatura grande */
        ssd1306_draw_string(0,  34, l_aux, 1);           /* pressao/umidade    */
        ssd1306_draw_string(4,  46, estado_txt(st), 2);  /* estado             */
        ssd1306_flush();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
