/*
 * buzzer - controle de um buzzer ativo por GPIO (liga/desliga).
 * Buzzer ativo ja gera o tom sozinho; basta energizar o pino.
 * Componente desacoplado de qualquer logica de incubadora.
 */
#pragma once
#include <stdbool.h>

/* Configura o pino como saida. active_high=true: nivel alto apita. */
void buzzer_init(int gpio, bool active_high);

/* Liga (apita) ou desliga o buzzer. */
void buzzer_set(bool on);
