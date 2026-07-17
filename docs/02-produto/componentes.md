# Componentes

O hardware da incubadora é formado por **dois sensores** (temperatura/pressão e umidade),
**três atuadores/sinalizadores** (relé do aquecedor, LED RGB e buzzer), um **display** e a
**conexão Wi-Fi**. Cada dispositivo é controlado por um **componente** próprio do ESP-IDF —
uma pasta em `incubadora/components/` com o *driver* daquele periférico.

Esses drivers não contêm nenhuma regra da incubadora: eles apenas sabem **conversar com o
dispositivo** (inicializar, ler, escrever). Toda a lógica (quando ligar o aquecedor, quando
apitar) fica nas tarefas, na pasta `main/`. Essa separação deixa cada driver **reutilizável**
em outros projetos e mantém o código organizado.

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
    diferenciados pelo **endereço** (0x76 e 0x3C). Assim, dois periféricos usam apenas dois
    pinos da ESP, em vez de quatro.

## Onde é definido na ESP

Todos os pinos, endereços e faixas ficam **centralizados** em um único arquivo,
[`incubadora/main/app_config.h`](https://github.com/FGA-FSE/Trabalho-3-Mayara-Raquel/blob/main/incubadora/main/app_config.h).
Para trocar um pino ou um limite, edita-se só esse arquivo — nenhum valo fica "espalhado"
pelo código:

```c
// I²C (BMP280 + OLED, barramento compartilhado)
#define I2C_SDA_GPIO        21
#define I2C_SCL_GPIO        22
#define I2C_FREQ_HZ         50000     // 50 kHz: estável em protoboard
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

## Sensores

### BMP280 — temperatura e pressão

É o sensor que fornece a **temperatura**, ou seja, a variável que o sistema controla. Ele se
comunica pelo barramento **I²C** e também mede a **pressão atmosférica**. O driver `bme280`
detecta o chip automaticamente, então funciona igual com um BMP280 ou um BME280.

??? info "Detalhe técnico — BMP280"
    - Endereço I²C **0x76** (pino SDO no GND; seria 0x77 com SDO no VCC).
    - Barramento a **50 kHz** — uma frequência conservadora, estável em protoboard e com
      fios longos.
    - O driver identifica o chip pelo registrador de *ID* (0x58 = BMP280, 0x60 = BME280) e lê
      a **calibração de fábrica** gravada no sensor para converter os valores brutos em °C e
      hPa.
    - A leitura é feita em **modo forçado** (*forced mode*): a cada ciclo dispara-se uma
      medição e lê-se o resultado, o que economiza energia.

### DHT11 — umidade

Sensor simples e barato que mede a **umidade relativa** do ar (em %). Ele usa um único fio de
dados (**GPIO32**) e é lido a cada 2 segundos. Por ser um protocolo sensível a tempo, a
leitura é feita com cuidado para não sofrer interferência das outras tarefas.

??? info "Detalhe técnico — DHT11"
    O DHT11 usa um protocolo **de 1 fio** (*single-wire*), com o timing em microssegundos: a
    ESP segura a linha em nível baixo por **≥ 18 ms** (sinal de *start*), o sensor responde e
    envia **40 bits** (umidade, temperatura e um *checksum*), codificados pela **largura dos
    pulsos**.

    Como esse timing é frágil, o driver lê os 40 bits dentro de uma **seção crítica** (com
    interrupções desligadas por ~5 ms) e valida o *checksum*; se falhar, a tarefa
    [`task_dht11`](funcoes.md) **tenta de novo** (até 3×) antes de marcar a umidade como
    inválida. Só é possível ler o DHT11 cerca de **uma vez por segundo**.

## Display

### OLED SSD1306

Display gráfico de **0,96"** (128 × 64 pixels) que mostra, na própria incubadora, a
temperatura, a umidade e o estado atual. Compartilha o barramento I²C com o BMP280
(endereço **0x3C**), então não gasta pinos adicionais.

## Atuadores e sinalização

### LED RGB — cor do estado

Um LED RGB indica visualmente a situação: **verde** dentro da faixa segura, **vermelho**
fora dela e **azul** durante a inicialização ou falha de sensor. O brilho de cada cor é
controlado por **PWM**.

??? info "Detalhe técnico — LED RGB"
    O controle usa o periférico **LEDC** do ESP32 (PWM por hardware), com **3 canais**
    (R/G/B), resolução de **8 bits** (duty de 0 a 255) e frequência de **5 kHz**. Como o
    módulo é de **ânodo comum** (acende no nível baixo), o driver **inverte o duty**
    internamente, de modo que quem chama o `rgb_led_set(r, g, b)` pensa sempre em "0 =
    apagado, 255 = máximo".

### Buzzer — alarme sonoro

Um **buzzer ativo** que apita quando a temperatura sai da faixa segura. Por ser ativo, basta
**energizar o pino** (GPIO33) para ele gerar o som — não precisa de PWM.

??? info "Detalhe técnico — buzzer"
    O buzzer é tratado como uma **saída digital**: nível **alto** (1) apita, nível baixo (0)
    silencia (`BUZZER_ATIVO_ALTO`). O padrão de apito é intermitente (250 ms ligado / 250 ms
    desligado), controlado pela tarefa [`task_buzzer`](funcoes.md).

### Relé — aquecedor

O **relé** é a chave que liga e desliga o **aquecedor**. É controlado pelo **GPIO19** e é o
único atuador de potência do sistema — é ele que efetivamente aquece a câmara.

??? info "Detalhe técnico — relé"
    O módulo tem **optoacoplador** e **aciona em nível baixo** (`RELE_ATIVO_BAIXO`): nível 0
    fecha o relé (liga o aquecedor), nível 1 solta. Por segurança, o firmware **inicializa o
    relé desligado** (`rele_init` coloca o pino em nível alto), garantindo que o aquecedor
    **não feche durante o boot**, antes de a tarefa de controle assumir.

!!! tip "Ponto de atenção no relé"
    O sinal do relé está no **GPIO19**. Se o LED do módulo não acender quando o firmware manda
    ligar, quase sempre é **pino ou polaridade** — ambos ajustáveis em uma linha do
    `app_config.h` (`RELE_GPIO` e `RELE_ATIVO_BAIXO`).

## Comunicação

### Wi-Fi + servidor HTTP

A ESP32 cria a sua **própria rede Wi-Fi** (modo *SoftAP*), sem precisar de roteador. Ao
conectar o celular nessa rede e abrir o navegador, o **servidor HTTP** embutido entrega uma
página com as leituras em tempo real. Os detalhes das rotas estão na página de
[Funções](funcoes.md).
