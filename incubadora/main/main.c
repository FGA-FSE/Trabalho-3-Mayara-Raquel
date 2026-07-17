/*
 * Incubadora - modulo de temperatura (etapa standalone).
 * Le BME280/BMP280, mostra no OLED, sinaliza no LED RGB e publica por Wi-Fi (SoftAP + HTTP).
 * Arquitetura: componentes desacoplados + tasks FreeRTOS sincronizadas por mutex (shared_state).
 */
#include "app_config.h"
#include "shared_state.h"
#include "tasks.h"
#include "bme280.h"
#include "ssd1306.h"
#include "rgb_led.h"
#include "dht11.h"
#include "buzzer.h"
#include "rele.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== Incubadora :: modulo de temperatura ===");

    /* 1) NVS (necessario para o Wi-Fi guardar calibracao/PHY) */
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* 2) Estado compartilhado (mutex) */
    shared_state_init();

    /* 3) LED RGB - acende azul (INIT) enquanto o resto sobe */
    rgb_led_init(LED_R_GPIO, LED_G_GPIO, LED_B_GPIO, LED_COMMON_ANODE);
    rgb_led_set(0, 0, 255);

    /* 3b) Atuadores - comecam em estado seguro (aquecedor desligado, buzzer mudo) */
    rele_init(RELE_GPIO, RELE_ATIVO_BAIXO);
    buzzer_init(BUZZER_GPIO, BUZZER_ATIVO_ALTO);

    /* 3c) Sensor de umidade DHT11 (1 fio) */
    dht11_init(DHT11_GPIO);

    /* 4) Barramento I2C (compartilhado por sensor + display) */
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    /* 5) Display OLED (opcional - segue sem ele se nao achar) */
    bool oled_ok = (ssd1306_init(bus, SSD1306_ADDR) == ESP_OK);
    if (oled_ok) {
        ssd1306_clear();
        ssd1306_draw_string(2, 20, "INICIANDO", 2);
        ssd1306_flush();
    } else {
        ESP_LOGW(TAG, "OLED nao encontrado em 0x%02X", SSD1306_ADDR);
    }

    /* 6) Sensor BME280/BMP280 */
    bme280_dev_t *dev = NULL;
    e = bme280_init(bus, BME280_ADDR, I2C_FREQ_HZ, &dev);
    if (e == ESP_OK)
        ESP_LOGI(TAG, "Sensor %s detectado em 0x%02X", bme280_chip_name(dev), BME280_ADDR);
    else
        ESP_LOGE(TAG, "Sensor nao encontrado (err=%s) - tasks seguem, status=ERRO",
                 esp_err_to_name(e));

    /* 7) Wi-Fi SoftAP + servidor HTTP (Wi-Fi antes do HTTP) */
    wifi_ap_start();
    http_server_start();

    /* 8) Tasks da aplicacao (loops) - sincronizadas via shared_state */
    xTaskCreate(task_sensor,     "sensor",  4096, dev,  5, NULL);
    xTaskCreate(task_dht11,      "dht11",   4096, NULL, 5, NULL);
    xTaskCreate(task_control,    "control", 3072, NULL, 5, NULL);
    xTaskCreate(task_display,    "display", 4096, NULL, 4, NULL);
    xTaskCreate(task_led_status, "led",     2048, NULL, 3, NULL);
    xTaskCreate(task_buzzer,     "buzzer",  2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "Inicializacao concluida.");
}
