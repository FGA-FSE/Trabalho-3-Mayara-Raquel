/*
 * rgb_led - controle de um LED RGB por PWM (LEDC do ESP-IDF).
 * 3 canais (R/G/B), duty 0..255 por canal, com suporte a anodo/catodo comum.
 * Componente desacoplado de qualquer logica de incubadora.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Configura os 3 canais LEDC nos GPIOs informados.
 * common_anode=true inverte o duty (LED de anodo comum acende em nivel baixo). */
void rgb_led_init(int gpio_r, int gpio_g, int gpio_b, bool common_anode);

/* Define a cor. Cada componente 0 (apagado) a 255 (maximo). */
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);
