/*
 * task_wifi_ap - sobe a ESP32 em modo SoftAP (cria a propria rede Wi-Fi).
 * Nao e um loop: e chamado uma vez na inicializacao.
 */
#include "tasks.h"
#include "app_config.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "wifi_ap";

void wifi_ap_start(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wc = {0};
    strncpy((char *)wc.ap.ssid, WIFI_AP_SSID, sizeof(wc.ap.ssid) - 1);
    wc.ap.ssid_len       = strlen(WIFI_AP_SSID);
    wc.ap.channel        = WIFI_AP_CHANNEL;
    wc.ap.max_connection = WIFI_AP_MAX_CONN;
    strncpy((char *)wc.ap.password, WIFI_AP_PASS, sizeof(wc.ap.password) - 1);
    wc.ap.authmode = (strlen(WIFI_AP_PASS) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP '%s' no ar. Conecte e acesse http://192.168.4.1/", WIFI_AP_SSID);
}
