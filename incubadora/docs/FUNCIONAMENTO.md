# Documentação de funcionamento

## 1. Visão geral

Este módulo lê um sensor ambiental (BME280/BMP280) por I2C, publica a leitura
em um estado compartilhado protegido por mutex e três consumidores reagem a
esse dado de forma independente: o **display OLED** mostra os valores, o **LED
RGB** sinaliza a faixa de temperatura por cor, e um **servidor HTTP** (sobre
Wi-Fi em modo SoftAP) expõe a leitura como página web e como JSON. A
arquitetura é modular (drivers em `components/`, lógica em tasks FreeRTOS) para
que os mesmos componentes sejam reaproveitados na incubadora completa.

## 2. Diagrama de blocos (fluxo de dados)

```
                         +---------------------+
   BME280/BMP280  --I2C--> task_sensor (prio 5) |
     (0x76)               |  lê a cada 2000 ms  |
                          +----------+----------+
                                     | escreve (mutex)
                                     v
                       +-----------------------------+
                       |     shared_state            |
                       |  sensor_data_t + SemaphoreMutex |
                       +--+-----------+-----------+--+
              lê (mutex)  |           |           |  lê (mutex)
            +-------------+     +-----+-----+     +--------------+
            v                   v           v                    v
   task_display (prio 4)  task_led_status  http /api/status   http /
   OLED a cada 1000 ms    (prio 3, 300 ms) (sob demanda)      (página HTML)
   SSD1306 via I2C        RGB via LEDC/PWM  JSON              auto-refresh 2s
```

O sensor é o **único produtor**; display, LED e HTTP são **consumidores**
somente-leitura. Ninguém acessa o sensor diretamente exceto a `task_sensor`.

## 3. Tasks

| Task | Prioridade | Período | O que faz | Recurso compartilhado / primitiva |
|---|---|---|---|---|
| `task_sensor` | 5 | 2000 ms | Dispara medição no BME280/BMP280 (modo forçado), monta `sensor_data_t` e publica | **escreve** em `shared_state` via **mutex** |
| `task_display` | 4 | 1000 ms | Lê o estado, formata e desenha no OLED (temp grande, pressão/umidade, estado) | **lê** `shared_state` via mutex |
| `task_led_status` | 3 | 300 ms | Deriva o status da temperatura e define a cor do LED (PWM) | **lê** `shared_state` via mutex |

Serviços de rede (não são tasks de aplicação, rodam em tasks internas do
ESP-IDF):

- **Wi-Fi SoftAP** (`task_wifi_ap.c` → `wifi_ap_start()`): sobe a stack Wi-Fi.
- **HTTP server** (`task_http_server.c` → `http_server_start()`): o
  `esp_http_server` cria sua própria task; os handlers leem `shared_state`.

A sincronização usa um **mutex** (`xSemaphoreCreateMutex`) porque há um
produtor e vários consumidores de uma estrutura pequena; não há necessidade de
fila ou event group nesta etapa (não há eventos/alarmes assíncronos ainda).

## 4. Componentes de hardware

### BME280 / BMP280 (`components/bme280`)
- **Protocolo**: I2C, endereço **0x76** (SDO→GND). `CSB→3V3` força o modo I2C.
- **Detecção automática**: lê o *chip id* (`0xD0`). `0x58` = BMP280
  (temp+pressão), `0x60` = BME280 (temp+pressão+umidade). A umidade só é lida
  e exposta quando há um BME280.
- **Leitura**: modo **forçado** — cada leitura dispara uma medição e lê os
  registradores; aplica a compensação oficial da Bosch (float).
- **Abstração**: `bme280_init()` devolve um handle opaco; `bme280_read()`
  preenche `bme280_reading_t`. Nenhuma dependência de I2C vaza para as tasks.

### Display OLED SSD1306 (`components/ssd1306_display`)
- **Protocolo**: I2C, endereço **0x3C**, mesmo barramento do sensor.
- **Funcionamento**: framebuffer de 1024 bytes na RAM; `ssd1306_draw_string()`
  desenha numa fonte 5x7 embutida (com escala) e `ssd1306_flush()` envia por
  páginas. O desenho da tela da incubadora fica em `task_display.c`.

### LED RGB WCMCU 5050 (`components/rgb_led`)
- **Protocolo**: 3 canais **LEDC/PWM** (5 kHz, 8 bits), GPIO 25/26/27.
- **Configuração**: **ânodo comum** — `rgb_led_init(..., true)` inverte o duty
  (o LED acende em nível baixo). Resistor em série obrigatório por canal.
- **Abstração**: `rgb_led_set(r,g,b)` recebe 0–255 por cor; a mistura é feita
  por PWM.

## 5. Comunicação Wi-Fi

- **Modo SoftAP**: SSID `IncubadoraAP`, senha `incubadora123`, canal 1,
  WPA2-PSK (definidos em `main/app_config.h`). IP fixo do gateway:
  **192.168.4.1**.
- **Endpoints HTTP** (`esp_http_server`):
  - `GET /` → página HTML (JS puro) com três cartões: (1) leitura atual com
    *badge* colorido (auto-refresh a cada 2 s), (2) **legenda** das cores/faixas
    (montada dinamicamente a partir dos limites atuais) e (3) **formulário para
    ajustar as faixas** em runtime.
  - `GET /api/status` → JSON:
    ```json
    {"temperature":30.12,"humidity":null,"pressure":1013.2,"status":"ok","sensor_ok":true}
    ```
    `humidity` é `null` no BMP280 (vira número no BME280). `status` é um de
    `ok | warning | alarm | error | init`.
  - `GET /api/config` → JSON com as faixas atuais:
    ```json
    {"ideal_min":28.00,"ideal_max":31.00,"banda":1.00}
    ```
  - `POST /api/config` (form-encoded `ideal_min=..&ideal_max=..&banda=..`) →
    valida (exige `ideal_min < ideal_max` e `banda >= 0`) e aplica na hora;
    responde com o mesmo JSON do `GET`. Os limites vivem no `shared_state`
    (protegidos por mutex) e começam com os valores de `app_config.h`; a
    alteração vale até o próximo reboot.

## 6. Máquina de estados do LED

Faixas relativas aos limites em `app_config.h`
(`TEMP_IDEAL_MIN`, `TEMP_IDEAL_MAX`, `TEMP_ALERTA_BANDA`):

| Condição | Estado | Cor |
|---|---|---|
| dentro de [IDEAL_MIN, IDEAL_MAX] | `ST_OK` | Verde |
| até ALERTA_BANDA fora da faixa ideal | `ST_WARNING` | Amarelo (R+G) |
| além disso | `ST_ALARM` | Vermelho |
| leitura do sensor falhou | `ST_ERROR` | Azul piscando |
| inicializando (antes da 1ª leitura) | `ST_INIT` | Azul fixo |

A transição é reavaliada a cada 300 ms por `task_led_status`, sempre a partir
do dado mais recente em `shared_state` (`compute_status()`). Os três limites
(`ideal_min`, `ideal_max`, `banda`) começam com os valores de `app_config.h`,
mas podem ser **ajustados em tempo real** pela página web (`POST /api/config`)
— a nova faixa passa a valer imediatamente para o LED, o OLED e a página.

## 7. Fluxo de inicialização (`app_main`)

A ordem importa:

1. **NVS** (`nvs_flash_init`) — o Wi-Fi guarda calibração/PHY na NVS; precisa
   estar pronto antes de subir a stack Wi-Fi.
2. **shared_state** (cria o mutex) — antes de qualquer task que o use.
3. **LED RGB** — acende **azul (INIT)** já, dando feedback visual imediato.
4. **Barramento I2C** — precisa existir antes de inicializar sensor e display.
5. **OLED** — mostra "INICIANDO"; segue mesmo se não for encontrado.
6. **Sensor** — detecta o chip; se falhar, as tasks seguem e o status vira
   ERRO (LED azul piscando).
7. **Wi-Fi (SoftAP) → HTTP server** — Wi-Fi **antes** do HTTP (o servidor
   depende da interface de rede já ativa).
8. **Tasks** — cria `task_sensor`, `task_display` e `task_led_status`.
