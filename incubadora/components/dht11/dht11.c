#include "dht11.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"      /* esp_rom_delay_us */
#include "esp_timer.h"        /* esp_timer_get_time */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static int s_gpio = -1;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

void dht11_init(int gpio)
{
    s_gpio = gpio;
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,  /* open-drain: podemos puxar p/ baixo e ler */
        .pull_up_en   = GPIO_PULLUP_ENABLE,         /* backup; o modulo DHT11 ja traz 10k */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(s_gpio, 1);   /* repouso: linha liberada (alta pelo pull-up) */
}

/* Espera a linha chegar em 'want' (0/1), medindo o tempo. Retorna a duracao em
 * microssegundos, ou -1 se estourar 'timeout_us'. */
static inline int wait_level(int want, int timeout_us)
{
    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(s_gpio) != want) {
        if ((esp_timer_get_time() - t0) > timeout_us)
            return -1;
    }
    return (int)(esp_timer_get_time() - t0);
}

esp_err_t dht11_read(dht11_reading_t *out)
{
    if (s_gpio < 0 || out == NULL)
        return ESP_ERR_INVALID_STATE;

    uint8_t b[5] = {0};

    /* 1) Sinal de start: segura a linha em nivel baixo por >= 18 ms.
     *    Busy-wait (e nao vTaskDelay) para garantir o minimo do datasheet
     *    independente do tick do FreeRTOS (10 ms); senao a leitura falha as vezes. */
    gpio_set_level(s_gpio, 0);
    esp_rom_delay_us(20000);

    /* 2) Timing critico (~5 ms, interrupcoes desligadas): libera a linha e ja le a
     *    resposta aqui dentro, para nenhuma ISR dessincronizar os bits. */
    portENTER_CRITICAL(&s_mux);
    gpio_set_level(s_gpio, 1);      /* libera; o pull-up levanta a linha */
    esp_rom_delay_us(30);

    /* Resposta do DHT: ~80us em nivel baixo, depois ~80us em nivel alto. */
    if (wait_level(0, 120) < 0) { portEXIT_CRITICAL(&s_mux); return ESP_ERR_TIMEOUT; }
    if (wait_level(1, 120) < 0) { portEXIT_CRITICAL(&s_mux); return ESP_ERR_TIMEOUT; }
    if (wait_level(0, 120) < 0) { portEXIT_CRITICAL(&s_mux); return ESP_ERR_TIMEOUT; }

    /* 40 bits: cada bit e um pulso baixo (~50us) seguido de um pulso alto.
     * alto curto (~26us) = 0 ; alto longo (~70us) = 1. */
    for (int i = 0; i < 40; i++) {
        if (wait_level(1, 90) < 0) { portEXIT_CRITICAL(&s_mux); return ESP_ERR_TIMEOUT; }
        int high_us = wait_level(0, 120);
        if (high_us < 0)           { portEXIT_CRITICAL(&s_mux); return ESP_ERR_TIMEOUT; }

        b[i >> 3] <<= 1;
        if (high_us > 50)          /* limiar entre bit 0 e bit 1 */
            b[i >> 3] |= 1;
    }

    portEXIT_CRITICAL(&s_mux);

    /* 3) Checksum: soma dos 4 primeiros bytes (8 bits) == 5o byte. */
    if ((uint8_t)(b[0] + b[1] + b[2] + b[3]) != b[4])
        return ESP_ERR_INVALID_CRC;

    out->humidity    = (float)b[0] + b[1] * 0.1f;
    out->temperature = (float)b[2] + (b[3] & 0x7F) * 0.1f;
    if (b[3] & 0x80)
        out->temperature = -out->temperature;

    return ESP_OK;
}
