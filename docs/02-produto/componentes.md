# Componentes

O hardware da incubadora é formado por **dois sensores**, **três atuadores/sinalizadores**,
um **display** e a **conexão Wi-Fi**. Cada dispositivo é controlado por um **componente**
próprio do ESP-IDF (uma pasta em `incubadora/components/`), sem lógica de incubadora dentro
dele — o que torna cada driver reutilizável.

## Visão geral

| Componente | Tipo | Ligação na ESP | O que faz | Pacote (driver) |
|---|---|---|---|---|
| **BMP280** | Sensor | I²C — SDA 21 / SCL 22 (0x76) | Mede **temperatura** e pressão | `bme280` |
| **DHT11** | Sensor | **GPIO32** (1 fio) | Mede **umidade** relativa | `dht11` |
| **OLED SSD1306** | Display | I²C — SDA 21 / SCL 22 (0x3C) | Mostra a leitura e o estado | `ssd1306_display` |
| **LED RGB** | Sinalização | PWM — R 25 / G 26 / B 27 | Cor conforme a temperatura | `rgb_led` |
| **Buzzer** | Sinalização | **GPIO33** (ativo-alto) | Alarme sonoro fora da faixa | `buzzer` |
| **Relé (aquecedor)** | Atuador | **GPIO19** (ativo-baixo) | Liga/desliga o aquecedor | `rele` |
| **Wi-Fi + HTTP** | Comunicação | SoftAP em `192.168.4.1` | Página web e API JSON | *(ESP-IDF)* |

!!! note "Barramento I²C compartilhado"
    O **BMP280** e o **OLED SSD1306** dividem o mesmo barramento I²C (pinos 21 e 22),
    diferenciados pelo endereço (0x76 e 0x3C). Isso economiza pinos da ESP.

## Onde é definido na ESP

Todos os pinos e faixas ficam **centralizados** em um único arquivo,
[`incubadora/main/app_config.h`](https://github.com/FGA-FSE/Trabalho-3-Mayara-Raquel/blob/main/incubadora/main/app_config.h) —
basta editar ali para trocar um pino ou um limite:

```c
// I²C (BMP280 + OLED, barramento compartilhado)
#define I2C_SDA_GPIO        21
#define I2C_SCL_GPIO        22
#define BME280_ADDR         0x76
#define SSD1306_ADDR        0x3C

// LED RGB (PWM via LEDC) — ânodo comum
#define LED_R_GPIO          25
#define LED_G_GPIO          26
#define LED_B_GPIO          27

// DHT11 (umidade)
#define DHT11_GPIO          32

// Buzzer (alarme sonoro) — aciona em nível ALTO
#define BUZZER_GPIO         33

// Relé (aquecedor) — módulo com opto, aciona em nível BAIXO
#define RELE_GPIO           19
```

## Detalhes por componente

- **BMP280** — sensor I²C de temperatura e pressão. O driver `bme280` detecta o chip
  automaticamente (funciona também com o BME280). A **temperatura** é a variável de controle
  do sistema.
- **DHT11** — sensor de umidade de 1 fio (protocolo *bit-banging*). Lido a cada 2 s, com o
  timing crítico protegido para não sofrer interferência de outras tarefas.
- **OLED SSD1306** — display de 0,96" que mostra temperatura, umidade e o estado atual.
- **LED RGB** — controlado por **PWM (LEDC)**. Como o módulo é de **ânodo comum**, o driver
  já inverte o sinal internamente.
- **Buzzer** — buzzer ativo (basta energizar o pino para apitar); usado como alarme sonoro.
- **Relé** — módulo com **optoacoplador**, que **aciona em nível baixo**. O firmware o deixa
  **desligado no boot** para o aquecedor não fechar durante a inicialização.

!!! tip "Pino do relé"
    O sinal do relé está no **GPIO19**. É um ponto de atenção: se o LED do módulo não acende
    quando o firmware manda ligar, quase sempre é pino ou polaridade — ambos ajustáveis em
    `app_config.h`.
