#include "shared_state.h"
#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include <string.h>

static sensor_data_t    s_data;
static temp_limits_t    s_limits;
static SemaphoreHandle_t s_mtx;

void shared_state_init(void)
{
    s_mtx = xSemaphoreCreateMutex();
    memset(&s_data, 0, sizeof(s_data));
    s_data.sensor_ok = false;
    /* faixas comecam com os valores de compilacao; a web pode alterar depois */
    s_limits.ideal_min    = TEMP_IDEAL_MIN;
    s_limits.ideal_max    = TEMP_IDEAL_MAX;
    s_limits.alerta_banda = TEMP_ALERTA_BANDA;
}

void shared_state_set_env(float temperature, float pressure, bool ok)
{
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    s_data.temperature = temperature;
    s_data.pressure    = pressure;
    s_data.sensor_ok   = ok;
    s_data.updated_ms  = (uint32_t)(esp_timer_get_time() / 1000);
    xSemaphoreGive(s_mtx);
}

void shared_state_set_humidity(float humidity, bool ok)
{
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    s_data.humidity     = humidity;
    s_data.has_humidity = ok;
    xSemaphoreGive(s_mtx);
}

void shared_state_set_heater(bool on)
{
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    s_data.heater_on = on;
    xSemaphoreGive(s_mtx);
}

void shared_state_get(sensor_data_t *out)
{
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    *out = s_data;
    xSemaphoreGive(s_mtx);
}

void shared_state_get_limits(temp_limits_t *out)
{
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    *out = s_limits;
    xSemaphoreGive(s_mtx);
}

bool shared_state_set_limits(const temp_limits_t *in)
{
    /* sanidade: faixa coerente e dentro do range util do sensor */
    if (!(in->ideal_min < in->ideal_max) || in->alerta_banda < 0.0f ||
        in->ideal_min < -40.0f || in->ideal_max > 85.0f)
        return false;

    xSemaphoreTake(s_mtx, portMAX_DELAY);
    s_limits = *in;
    xSemaphoreGive(s_mtx);
    return true;
}

incub_status_t compute_status(const sensor_data_t *d)
{
    if (!d->sensor_ok) return ST_ERROR;

    temp_limits_t lim;
    shared_state_get_limits(&lim);   /* usa as faixas atuais (ajustaveis via web) */

    float t = d->temperature;
    if (t >= lim.ideal_min && t <= lim.ideal_max) return ST_OK;
    if (t >= lim.ideal_min - lim.alerta_banda &&
        t <= lim.ideal_max + lim.alerta_banda) return ST_WARNING;
    return ST_ALARM;
}

const char *status_str(incub_status_t s)
{
    switch (s) {
        case ST_OK:      return "ok";
        case ST_WARNING: return "warning";
        case ST_ALARM:   return "alarm";
        case ST_ERROR:   return "error";
        default:         return "init";
    }
}
