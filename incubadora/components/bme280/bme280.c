#include "bme280.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMEOUT_MS      1000

/* registradores */
#define REG_ID          0xD0
#define REG_RESET       0xE0
#define REG_CTRL_HUM    0xF2
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_DATA        0xF7   /* press(3) temp(3) hum(2) */
#define REG_CALIB_TP    0x88   /* 26 bytes (0x88..0xA1) */
#define REG_CALIB_H     0xE1   /* 7 bytes (BME280)      */

#define CHIP_BMP280     0x58
#define CHIP_BME280     0x60

struct bme280_dev_s {
    i2c_master_dev_handle_t h;
    uint8_t  chip;
    bool     has_hum;
    /* calibracao de temperatura/pressao */
    uint16_t T1;  int16_t T2, T3;
    uint16_t P1;  int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    /* calibracao de umidade (BME280) */
    uint8_t  H1, H3;  int16_t H2, H4, H5;  int8_t H6;
    int32_t  t_fine;
};

static esp_err_t rd(bme280_dev_t *d, uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(d->h, &reg, 1, buf, n, TIMEOUT_MS);
}

static esp_err_t wr(bme280_dev_t *d, uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(d->h, b, sizeof(b), TIMEOUT_MS);
}

static void read_calibration(bme280_dev_t *d)
{
    uint8_t t[26] = {0};
    rd(d, REG_CALIB_TP, t, sizeof(t));
    d->T1 = (uint16_t)(t[1]  << 8 | t[0]);
    d->T2 = (int16_t) (t[3]  << 8 | t[2]);
    d->T3 = (int16_t) (t[5]  << 8 | t[4]);
    d->P1 = (uint16_t)(t[7]  << 8 | t[6]);
    d->P2 = (int16_t) (t[9]  << 8 | t[8]);
    d->P3 = (int16_t) (t[11] << 8 | t[10]);
    d->P4 = (int16_t) (t[13] << 8 | t[12]);
    d->P5 = (int16_t) (t[15] << 8 | t[14]);
    d->P6 = (int16_t) (t[17] << 8 | t[16]);
    d->P7 = (int16_t) (t[19] << 8 | t[18]);
    d->P8 = (int16_t) (t[21] << 8 | t[20]);
    d->P9 = (int16_t) (t[23] << 8 | t[22]);
    d->H1 = t[25];   /* 0xA1 */

    if (d->has_hum) {
        uint8_t h[7] = {0};
        rd(d, REG_CALIB_H, h, sizeof(h));
        d->H2 = (int16_t)(h[1] << 8 | h[0]);
        d->H3 = h[2];
        d->H4 = (int16_t)(((int8_t)h[3] << 4) | (h[4] & 0x0F));
        d->H5 = (int16_t)(((int8_t)h[5] << 4) | (h[4] >> 4));
        d->H6 = (int8_t)h[6];
    }
}

/* Formulas de compensacao da Bosch (versao float) */
static float comp_temp(bme280_dev_t *d, int32_t adc_T)
{
    float v1 = ((float)adc_T / 16384.0f  - (float)d->T1 / 1024.0f) * (float)d->T2;
    float v2 = (((float)adc_T / 131072.0f - (float)d->T1 / 8192.0f) *
                ((float)adc_T / 131072.0f - (float)d->T1 / 8192.0f)) * (float)d->T3;
    d->t_fine = (int32_t)(v1 + v2);
    return (v1 + v2) / 5120.0f;
}

static float comp_press(bme280_dev_t *d, int32_t adc_P)
{
    float v1 = ((float)d->t_fine / 2.0f) - 64000.0f;
    float v2 = v1 * v1 * (float)d->P6 / 32768.0f;
    v2 = v2 + v1 * (float)d->P5 * 2.0f;
    v2 = (v2 / 4.0f) + ((float)d->P4 * 65536.0f);
    v1 = ((float)d->P3 * v1 * v1 / 524288.0f + (float)d->P2 * v1) / 524288.0f;
    v1 = (1.0f + v1 / 32768.0f) * (float)d->P1;
    if (v1 == 0.0f) return 0.0f;
    float p = 1048576.0f - (float)adc_P;
    p = (p - (v2 / 4096.0f)) * 6250.0f / v1;
    v1 = (float)d->P9 * p * p / 2147483648.0f;
    v2 = p * (float)d->P8 / 32768.0f;
    p = p + (v1 + v2 + (float)d->P7) / 16.0f;
    return p / 100.0f;   /* Pa -> hPa */
}

static float comp_hum(bme280_dev_t *d, int32_t adc_H)
{
    float h = ((float)d->t_fine - 76800.0f);
    h = ((float)adc_H - ((float)d->H4 * 64.0f + (float)d->H5 / 16384.0f * h)) *
        ((float)d->H2 / 65536.0f * (1.0f + (float)d->H6 / 67108864.0f * h *
        (1.0f + (float)d->H3 / 67108864.0f * h)));
    h = h * (1.0f - (float)d->H1 * h / 524288.0f);
    if (h > 100.0f) h = 100.0f;
    else if (h < 0.0f) h = 0.0f;
    return h;
}

esp_err_t bme280_init(i2c_master_bus_handle_t bus, uint8_t addr, uint32_t scl_hz, bme280_dev_t **out)
{
    bme280_dev_t *d = calloc(1, sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = scl_hz,
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &cfg, &d->h);
    if (e != ESP_OK) { free(d); return e; }

    uint8_t id = 0;
    e = rd(d, REG_ID, &id, 1);
    if (e != ESP_OK)                              { i2c_master_bus_rm_device(d->h); free(d); return e; }
    if (id != CHIP_BMP280 && id != CHIP_BME280)   { i2c_master_bus_rm_device(d->h); free(d); return ESP_ERR_NOT_FOUND; }

    d->chip    = id;
    d->has_hum = (id == CHIP_BME280);

    wr(d, REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));
    read_calibration(d);

    if (d->has_hum) wr(d, REG_CTRL_HUM, 0x01);   /* osrs_h = x1 (aplicar antes de ctrl_meas) */
    wr(d, REG_CONFIG, 0x00);                     /* filtro desligado */

    *out = d;
    return ESP_OK;
}

esp_err_t bme280_read(bme280_dev_t *d, bme280_reading_t *r)
{
    if (!d || !r) return ESP_ERR_INVALID_ARG;

    if (d->has_hum) wr(d, REG_CTRL_HUM, 0x01);
    esp_err_t e = wr(d, REG_CTRL_MEAS, 0x25);    /* osrs_t=x1, osrs_p=x1, modo=forcado */
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t b[8] = {0};
    size_t n = d->has_hum ? 8 : 6;
    e = rd(d, REG_DATA, b, n);
    if (e != ESP_OK) return e;

    int32_t adc_P = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | (b[2] >> 4);
    int32_t adc_T = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | (b[5] >> 4);

    r->temperature = comp_temp(d, adc_T);   /* calcula t_fine (usado por press/hum) */
    r->pressure    = comp_press(d, adc_P);
    if (d->has_hum) {
        int32_t adc_H = ((int32_t)b[6] << 8) | b[7];
        r->humidity     = comp_hum(d, adc_H);
        r->has_humidity = true;
    } else {
        r->humidity     = 0.0f;
        r->has_humidity = false;
    }
    return ESP_OK;
}

const char *bme280_chip_name(const bme280_dev_t *d)
{
    return (d && d->chip == CHIP_BME280) ? "BME280" : "BMP280";
}
