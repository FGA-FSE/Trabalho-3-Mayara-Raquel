# Rastreabilidade de requisitos

Mapeia cada requisito do enunciado para onde é atendido no código deste
módulo (etapa standalone de temperatura).

## Requisitos obrigatórios do enunciado

| ID | Requisito (enunciado) | Status | Onde é atendido |
|----|---|---|---|
| R01 | ESP32 como controlador central | ✅ | Projeto inteiro roda na ESP32 via ESP-IDF (`main/main.c`) |
| R02 | ESP-IDF puro (sem Arduino) | ✅ | `CMakeLists.txt` + `idf_component_register`; nenhum `#include <Arduino.h>` |
| R03 | FreeRTOS coordenando tasks | ✅ | `main.c` cria 3 tasks de aplicação (`task_sensor`/`task_display`/`task_led_status`); sincronização por **mutex** em `shared_state.c`. Wi-Fi e HTTP rodam como serviços com tasks internas do ESP-IDF |
| R04 | Sensores (entradas) | ✅ | BME280/BMP280 lido em `task_sensor.c` via componente `components/bme280` (temp/pressão, e umidade se BME280) |
| R05 | Atuadores (saídas) | ✅ | LED RGB (PWM/LEDC) em `task_led_status.c` via `components/rgb_led`; OLED em `task_display.c` via `components/ssd1306_display` |
| R06 | Comunicação wireless | ✅ | Wi-Fi **SoftAP** em `task_wifi_ap.c` + servidor HTTP em `task_http_server.c` (`GET /` e `GET /api/status`) |
| R07 | Qualidade do código | ✅ | Modularização em `components/`, tasks separadas, `shared_state` com mutex, comentários em PT-BR |
| R08 | README completo | ✅ | `README.md` (quick start, arquitetura, pinout) + `docs/` |
| R09 | Vídeo de demonstração | 🔲 | A gravar após validação final em hardware |

## Requisitos opcionais / pontos extra

| ID | Requisito | Status | Onde é atendido |
|----|---|---|---|
| E01 | Thingsboard (MQTT telemetria + RPC) | 🔲 | Planejado para a integração com a incubadora final |
| E02 | Comunicação Bluetooth | 🔲 | Não planejado nesta etapa |
| E03 | Qualidade/usabilidade acima da média | 🟡 | Página web com auto-refresh e badge colorido de status (`task_http_server.c`); LED informativo com faixas |
| E04 | Cadastro fácil de credenciais Wi-Fi | 🔲 | Planejado via `wifi_provisioning` (BLE/SoftAP) na integração |

## Requisitos derivados (decisões de projeto)

| ID | Requisito interno | Justificativa |
|----|---|---|
| D01 | Componentes desacoplados da lógica de incubadora | Reaproveitamento direto na integração final |
| D02 | Sensor e display no mesmo barramento I2C | Economia de pinos; ambos suportam multi-device (0x76 e 0x3C) |
| D03 | LED com PWM (LEDC), não digital on/off | Permite mistura de cores e transições suaves |
| D04 | SoftAP em vez de STA nesta etapa | Independência de infraestrutura de rede para o teste isolado |
| D05 | Makefile de conveniência | Facilitar build/flash/monitor sem decorar comandos |
| D06 | Driver de sensor próprio com auto-detecção BMP280/BME280 | O módulo físico disponível reportou chip id `0x58` (BMP280, sem umidade). O driver detecta o chip e habilita umidade só no BME280 (`0x60`), sem quebrar o hardware atual |
| D07 | Drivers próprios (novo `i2c_master`) em vez de `esp-idf-lib` | O projeto usa ESP-IDF v6.0.2, cujo driver I2C novo é incompatível com libs baseadas no I2C legado |
| D08 | LED configurado como **ânodo comum** | O módulo WCMCU 5050 disponível é ânodo comum (verificado em bancada); ver `LED_COMMON_ANODE` em `app_config.h` |

## Notas de hardware desta montagem

- **Sensor**: o módulo em mãos é um **BMP280** (chip id `0x58`) — mede
  temperatura e pressão, **não** umidade. O código já suporta o **BME280**
  (`0x60`): basta trocar o módulo que a umidade passa a ser lida e exposta.
- **LED RGB**: **ânodo comum** — pino comum vai no **3V3**. Resistor em
  série (220–330 Ω) em cada linha R/G/B é obrigatório (módulo sem resistor).
