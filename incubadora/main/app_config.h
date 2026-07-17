/*
 * app_config.h - Ajustes centrais do modulo de temperatura.
 * >>> Edite aqui: faixas de temperatura, pinos e credenciais do Wi-Fi. <<<
 */
#pragma once
#include <stdbool.h>

/* ===================== I2C (BME280 + OLED, barramento compartilhado) ===================== */
#define I2C_SDA_GPIO        21
#define I2C_SCL_GPIO        22
#define I2C_FREQ_HZ         50000     /* 50 kHz: estavel em protoboard */
#define BME280_ADDR         0x76      /* SDO->GND. Use 0x77 se SDO->VCC */
#define SSD1306_ADDR        0x3C      /* endereco tipico do OLED 0.96"  */

/* ===================== LED RGB (PWM via LEDC) ===================== */
#define LED_R_GPIO          25
#define LED_G_GPIO          26
#define LED_B_GPIO          27
#define LED_COMMON_ANODE    true      /* nosso modulo WCMCU 5050 e anodo comum */

/* ===================== DHT11 (umidade) ===================== */
#define DHT11_GPIO          32        /* dados do DHT11 no D32 */

/* ===================== Buzzer (alarme sonoro) ===================== */
#define BUZZER_GPIO         33        /* buzzer no D33 (saida digital) */
#define BUZZER_ATIVO_ALTO   true      /* buzzer ativo: nivel alto apita */

/* ===================== Rele (aquecedor) ===================== */
#define RELE_GPIO           19        /* sinal do rele no D19 (confirmado na bancada) */
#define RELE_ATIVO_BAIXO    true      /* modulo com optoacoplador: nivel baixo aciona */

/* ===================== FAIXAS DE ALARME / EXIBICAO (graus C) =====================
 *   Definem o LED de status, o buzzer e o estado mostrado (NAO o aquecedor):
 *   dentro de [IDEAL_MIN, IDEAL_MAX] ............... OK      (verde)
 *   ate ALERTA_BANDA fora da faixa ideal ........... ALERTA  (amarelo)
 *   alem disso ..................................... ALARME  (vermelho + buzzer)
 * Com os valores abaixo: verde de 27 a 32; buzzer/vermelho em T < 27 ou T > 32.
 */
#define TEMP_IDEAL_MIN      28.0f
#define TEMP_IDEAL_MAX      31.0f
#define TEMP_ALERTA_BANDA   1.0f

/* ===================== CONTROLE DO AQUECEDOR (histerese) =====================
 * Independente das faixas de alarme acima. O rele:
 *   LIGA    o aquecedor quando T <= HEATER_ON_BELOW_C
 *   DESLIGA o aquecedor quando T >= HEATER_OFF_ABOVE_C
 * Banda estreita (28..29) segura a temperatura sem superaquecer.
 */
#define HEATER_ON_BELOW_C   28.0f
#define HEATER_OFF_ABOVE_C  29.0f

/* ===================== Amostragem ===================== */
#define SENSOR_PERIODO_MS   2000      /* leitura de temperatura/pressao (BMP280) */
#define DHT11_PERIODO_MS    2000      /* leitura de umidade (DHT11; min ~1 s)    */
#define CONTROL_PERIODO_MS  1000      /* passo do controle do aquecedor (rele)   */

/* ===================== Wi-Fi (SoftAP) ===================== */
#define WIFI_AP_SSID        "IncubadoraAP"
#define WIFI_AP_PASS        "incubadora123"   /* min 8 caracteres; "" = rede aberta */
#define WIFI_AP_CHANNEL     1
#define WIFI_AP_MAX_CONN    4
