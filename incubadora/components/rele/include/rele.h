/*
 * rele - controle de um canal de rele por GPIO (liga/desliga).
 * Modulos rele com optoacoplador costumam ser ATIVOS EM NIVEL BAIXO.
 * Componente desacoplado de qualquer logica de incubadora.
 */
#pragma once
#include <stdbool.h>

/* Configura o pino como saida e deixa o rele DESLIGADO.
 * active_low=true: nivel baixo aciona a bobina (padrao dos modulos com opto). */
void rele_init(int gpio, bool active_low);

/* Liga (aciona) ou desliga o rele. */
void rele_set(bool on);
