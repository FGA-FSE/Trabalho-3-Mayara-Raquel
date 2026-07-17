/*
 * tasks.h - pontos de entrada das tasks FreeRTOS e dos servicos.
 */
#pragma once

/* Tasks (loops) criadas com xTaskCreate em main.c */
void task_sensor(void *arg);       /* arg = bme280_dev_t*  (temperatura/pressao) */
void task_dht11(void *arg);        /* umidade (DHT11)                            */
void task_display(void *arg);
void task_led_status(void *arg);
void task_buzzer(void *arg);       /* alarme sonoro fora da faixa                */
void task_control(void *arg);      /* histerese do aquecedor (rele)             */

/* Servicos de rede (sobem seus proprios contextos internos) */
void wifi_ap_start(void);
void http_server_start(void);
