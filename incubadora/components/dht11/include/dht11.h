/*
 * dht11 - driver para o sensor de umidade/temperatura DHT11 (1 fio).
 * Protocolo single-wire bit-banging. Componente desacoplado da incubadora.
 *
 * O DHT11 so pode ser lido a cada ~1-2 s. Apos ligar, espere ~1 s antes
 * da primeira leitura para o sensor estabilizar.
 */
#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    float humidity;      /* %RH (resolucao de 1%)      */
    float temperature;   /* graus C (resolucao de 1 C) */
} dht11_reading_t;

/* Configura o pino (open-drain com pull-up) e deixa a linha em repouso. */
void dht11_init(int gpio);

/* Faz uma leitura. Retorna:
 *   ESP_OK              leitura valida em *out
 *   ESP_ERR_TIMEOUT     sensor nao respondeu no tempo esperado
 *   ESP_ERR_INVALID_CRC checksum nao bateu (ruido na linha) */
esp_err_t dht11_read(dht11_reading_t *out);
