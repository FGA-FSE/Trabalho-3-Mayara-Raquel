# Incubadora — Módulo de Temperatura (ESP32 + ESP-IDF)

Módulo **standalone** da etapa inicial do trabalho de Fundamentos de Sistemas
Embarcados: lê um sensor ambiental, mostra a leitura em um OLED, sinaliza a
faixa de temperatura em um LED RGB e publica os dados por **Wi-Fi** (a própria
ESP32 sobe um Access Point + servidor HTTP local).

Construído em **ESP-IDF puro** (sem Arduino), com **FreeRTOS** (tasks +
mutex) e arquitetura **modular** — os drivers em `components/` são pensados
para serem reaproveitados na incubadora completa depois.

> **Hardware desta montagem:** o módulo de sensor disponível é um **BMP280**
> (temperatura + pressão, sem umidade). O código detecta o chip
> automaticamente e já suporta **BME280** (com umidade) — basta trocar o
> módulo. O LED RGB é **ânodo comum**.

---

## Como rodar

### 1. Pré-requisitos
É necessário ter o pacote **ESP-IDF v6.0.2** (ou v5.4+) instalado no sistema. Caso ainda não tenha o ambiente configurado, siga o passo a passo do [Guia Oficial de Instalação da Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).

### 2. Ativação do Ambiente
Antes de rodar os comandos do projeto, você precisa carregar as ferramentas do ESP-IDF no seu terminal. Execute o comando de ativação de acordo com o seu caminho de instalação:

```bash
# Utilizando o caminho absoluto (ajuste 'usuario' para o nome do seu usuário no Linux)
source /home/usuario/.espressif/tools/activate_idf_v6.0.2.sh

# Ou utilizando o atalho '~' (caso esteja na pasta home do usuário atual)
. ~/.espressif/tools/activate_idf_v6.0.2.sh
```

**Dica**: Se você configurou um alias durante a instalação oficial, pode apenas executar get_idf no terminal para ativar o ambiente. Mais detalhes e troubleshooting estão disponíveis em docs/SETUP.md.

### 3. Comandos para Rodar
Com o ambiente devidamente ativo no terminal, execute:

```bash
make set-target      # executado uma vez após clonar (configura o target para esp32)
make fm              # atalho para: compila (build), grava (flash) e abre o monitor serial
```

Porta diferente? `make fm PORT=/dev/ttyACM0`. Veja todos os atalhos com
`make help`.

Depois de gravar: conecte-se ao Wi-Fi **`IncubadoraAP`** (senha
`incubadora123`) e abra **http://192.168.4.1/** no navegador.

---

## Pinout

| Sinal | GPIO | Observação |
|---|---|---|
| I2C SDA (BME280 + OLED) | 21 | barramento compartilhado |
| I2C SCL (BME280 + OLED) | 22 | barramento compartilhado |
| LED R / G / B | 25 / 26 / 27 | LEDC PWM, **resistor 220–330 Ω em série** |

Sensor: `CSB→3V3` (modo I2C), `SDO→GND` (endereço `0x76`). OLED em `0x3C`.
LED RGB **ânodo comum**: pino comum → **3V3**. Detalhes e avisos de segurança
em [`docs/SETUP.md`](docs/SETUP.md).

---

## Arquitetura

```
incubadora/
├── Makefile                 # atalhos: build / flash / monitor / fm ...
├── sdkconfig.defaults       # flash 4MB + partição de app grande
├── components/
│   ├── bme280/              # driver I2C (auto-detecta BMP280/BME280)
│   ├── ssd1306_display/     # driver do OLED (framebuffer + fonte)
│   └── rgb_led/             # controle RGB por PWM (LEDC)
├── main/
│   ├── main.c               # inicialização + criação das tasks
│   ├── app_config.h         # >>> AJUSTES: faixas, pinos, Wi-Fi <<<
│   ├── shared_state.[ch]    # dado do sensor protegido por mutex
│   ├── task_sensor.c        # lê o sensor (prio 5, 2 s)
│   ├── task_display.c       # desenha no OLED (prio 4, 1 s)
│   ├── task_led_status.c    # cor do LED por faixa (prio 3, 0.3 s)
│   ├── task_wifi_ap.c       # sobe o SoftAP
│   └── task_http_server.c   # serve JSON + página web
└── docs/
    ├── SETUP.md             # guia do zero (instalação, fiação, testes)
    ├── FUNCIONAMENTO.md     # tasks, fluxo de dados, hardware, Wi-Fi
    └── REQUISITOS.md        # rastreabilidade dos requisitos do enunciado
```

Fluxo: `task_sensor` publica em `shared_state` (mutex); `task_display`,
`task_led_status` e o servidor HTTP consomem esse estado. Ninguém acessa o
sensor diretamente exceto a task do sensor.

---

## Ajustes rápidos

Tudo que costuma mudar está em [`main/app_config.h`](main/app_config.h):

- **Faixas de temperatura**: `TEMP_IDEAL_MIN`, `TEMP_IDEAL_MAX`,
  `TEMP_ALERTA_BANDA`.
- **Pinos** do I2C e do LED RGB.
- **Ânodo/catodo comum** do LED: `LED_COMMON_ANODE`.
- **Credenciais do Wi-Fi**: `WIFI_AP_SSID`, `WIFI_AP_PASS`.

---

## API HTTP

- `GET /` — página HTML com auto-refresh (2 s).
- `GET /api/status` — JSON:
  `{"temperature":30.12,"humidity":null,"pressure":1013.2,"status":"ok","sensor_ok":true}`
  (`humidity` = `null` no BMP280; `status` ∈ `ok|warning|alarm|error|init`).

---

## Documentação

- [`docs/SETUP.md`](docs/SETUP.md) — **comece aqui**: instalação do ESP-IDF,
  montagem do hardware, como compilar/gravar/testar, troubleshooting.
- [`docs/FUNCIONAMENTO.md`](docs/FUNCIONAMENTO.md) — como o software funciona
  (tasks, sincronização, máquina de estados do LED, fluxo de init).
- [`docs/REQUISITOS.md`](docs/REQUISITOS.md) — rastreabilidade dos requisitos.

---

## Próximos passos (evolução para a incubadora final)

- Trocar SoftAP por Wi-Fi **STA** + cliente **MQTT/Thingsboard** (telemetria +
  RPC), mantendo o HTTP local como debug.
- Provisionamento de credenciais Wi-Fi (`wifi_provisioning` via BLE/SoftAP).
- Consumir a mesma leitura de `shared_state` para o **controle** (relé de
  aquecimento, bomba), reaproveitando os componentes deste módulo.
