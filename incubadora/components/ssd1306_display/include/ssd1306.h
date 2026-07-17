/*
 * ssd1306 - driver minimo para OLED 128x64 I2C (driver novo i2c_master).
 * Framebuffer interno: desenhe e depois chame ssd1306_flush().
 * Componente desacoplado de qualquer logica de incubadora.
 */
#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"

/* Inicializa o display no barramento 'bus' (tipicamente addr 0x3C). */
esp_err_t ssd1306_init(i2c_master_bus_handle_t bus, uint8_t addr);

/* Limpa o framebuffer (nao envia ainda). */
void ssd1306_clear(void);

/* Escreve texto no framebuffer. x,y em pixels; scale=1 normal, 2 dobro. */
void ssd1306_draw_string(int x, int y, const char *s, int scale);

/* Envia o framebuffer para a tela. */
void ssd1306_flush(void);
