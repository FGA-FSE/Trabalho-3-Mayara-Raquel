/*
 * shared_state - dado de leitura do sensor protegido por mutex.
 * Ponto unico de verdade: a task do sensor publica aqui; display, LED e
 * HTTP consomem. Sem variaveis globais soltas.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float    temperature;   /* graus C (BMP280)                          */
    float    pressure;      /* hPa (BMP280)                              */
    float    humidity;      /* %RH (DHT11)                               */
    bool     has_humidity;  /* true se a ultima leitura do DHT11 foi ok  */
    bool     sensor_ok;     /* true se a ultima leitura de temperatura ok*/
    bool     heater_on;     /* estado atual do aquecedor (rele)          */
    uint32_t updated_ms;    /* instante da ultima atualizacao (ms desde o boot) */
} sensor_data_t;

/* Estado derivado (nao e armazenado; e calculado a partir do dado bruto) */
typedef enum {
    ST_INIT = 0,   /* inicializando            -> azul fixo      */
    ST_OK,         /* dentro da faixa ideal    -> verde          */
    ST_WARNING,    /* banda de alerta          -> amarelo        */
    ST_ALARM,      /* fora da faixa            -> vermelho       */
    ST_ERROR,      /* falha de leitura         -> azul piscando  */
} incub_status_t;

/* Faixas de temperatura ajustaveis em runtime (via web).
 * Inicializadas com os defaults de app_config.h em shared_state_init(). */
typedef struct {
    float ideal_min;      /* limite inferior da faixa IDEAL (graus C) */
    float ideal_max;      /* limite superior da faixa IDEAL (graus C) */
    float alerta_banda;   /* margem de ALERTA fora da faixa ideal (graus C) */
} temp_limits_t;

void            shared_state_init(void);
void            shared_state_get(sensor_data_t *out);

/* Escritores segmentados: cada produtor atualiza so a sua parte, sem
 * sobrescrever os campos dos outros (varios sensores publicam no mesmo estado). */
void            shared_state_set_env(float temperature, float pressure, bool ok); /* BMP280   */
void            shared_state_set_humidity(float humidity, bool ok);               /* DHT11    */
void            shared_state_set_heater(bool on);                                  /* controle */

/* Faixas de temperatura (protegidas pelo mesmo mutex do estado). */
void            shared_state_get_limits(temp_limits_t *out);
/* Valida e aplica; retorna false se invalido (exige min<max e banda>=0). */
bool            shared_state_set_limits(const temp_limits_t *in);

/* Traduz o dado bruto em estado usando as faixas atuais do shared_state. */
incub_status_t  compute_status(const sensor_data_t *d);
/* Nome curto do status para JSON: "ok"/"warning"/"alarm"/"error"/"init". */
const char     *status_str(incub_status_t s);
