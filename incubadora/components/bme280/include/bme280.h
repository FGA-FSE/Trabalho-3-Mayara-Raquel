/*
 * bme280 - driver I2C para BMP280 / BME280 (ESP-IDF, driver novo i2c_master).
 *
 * Detecta o chip automaticamente:
 *   - BMP280 (0x58): temperatura + pressao.
 *   - BME280 (0x60): temperatura + pressao + umidade.
 *
 * Componente desacoplado de qualquer logica de incubadora (reaproveitavel).
 */
#pragma once
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct bme280_dev_s bme280_dev_t;   /* handle opaco */

typedef struct {
    float temperature;   /* graus Celsius                         */
    float pressure;      /* hPa                                   */
    float humidity;      /* %RH (valido apenas se has_humidity)   */
    bool  has_humidity;  /* true se o chip for BME280             */
} bme280_reading_t;

/* Adiciona o sensor ao barramento, detecta o chip e le a calibracao de fabrica.
 * addr: 0x76 (SDO->GND) ou 0x77 (SDO->VCC). scl_hz: velocidade I2C (ex.: 50000).
 * Em sucesso, *out recebe o handle. Retorna ESP_ERR_NOT_FOUND se nao for BMP/BME280. */
esp_err_t bme280_init(i2c_master_bus_handle_t bus, uint8_t addr, uint32_t scl_hz, bme280_dev_t **out);

/* Dispara uma medicao (modo forcado) e preenche 'r'. */
esp_err_t bme280_read(bme280_dev_t *dev, bme280_reading_t *r);

/* Nome do chip detectado: "BMP280" ou "BME280". */
const char *bme280_chip_name(const bme280_dev_t *dev);
