# Funções

O firmware é dividido em **tarefas do FreeRTOS** — cada uma cuida de uma responsabilidade e
roda em paralelo. Todas leem/escrevem no **estado compartilhado** (ver
[Arquitetura](../03-analise-tecnica/arquitetura.md)); nenhuma conversa diretamente com a outra.

## Tarefas do sistema

| Tarefa (função) | O que faz | Período | Arquivo |
|---|---|---|---|
| `task_sensor` | Lê **temperatura** e pressão do BMP280 | 2 s | `task_sensor.c` |
| `task_dht11` | Lê a **umidade** do DHT11 | 2 s | `task_dht11.c` |
| `task_control` | **Liga/desliga o aquecedor** por histerese | 1 s | `task_control.c` |
| `task_led_status` | Define a **cor do LED** conforme a temperatura | 0,3 s | `task_led_status.c` |
| `task_buzzer` | **Apita** quando a temperatura sai da faixa | 0,25 s | `task_buzzer.c` |
| `task_display` | Desenha a leitura e o estado no **OLED** | 1 s | `task_display.c` |
| Wi-Fi + HTTP | Sobe o **SoftAP** e o **servidor web** | serviço | `task_wifi_ap.c` · `task_http_server.c` |

## Controle do aquecedor (histerese)

A `task_control` mantém a temperatura numa faixa estreita **ligando e desligando o relé**.
Não é um liga/desliga simples: usa **histerese** (banda morta) para o aquecedor não ficar
chaveando o tempo todo.

- **Liga** o aquecedor quando a temperatura **cai a 28 °C**;
- **Desliga** quando a temperatura **atinge 29 °C**;
- entre 28 e 29 °C, **mantém o estado atual**.

```c
if (heating) {
    if (temperatura >= HEATER_OFF_ABOVE_C)   // 29 °C -> desliga
        heating = false;
} else {
    if (temperatura <= HEATER_ON_BELOW_C)    // 28 °C -> liga
        heating = true;
}
```

!!! warning "Segurança"
    Se a leitura de temperatura **falhar**, a `task_control` **desliga o aquecedor** por
    segurança, evitando aquecer sem controle.

## Alarme e sinalização

O **LED RGB** e o **buzzer** avisam se a temperatura saiu da **faixa segura (27 a 32 °C)** —
independente do controle do aquecedor:

| Situação | Temperatura | LED | Buzzer | Aquecedor |
|---|---|---|---|---|
| Muito frio | **< 27 °C** | 🔴 vermelho | apitando | ligado |
| Faixa segura | **27 – 32 °C** | 🟢 verde | silêncio | histerese 28 ↔ 29 °C |
| Muito quente | **> 32 °C** | 🔴 vermelho | apitando | desligado |
| Inicializando / falha de sensor | — | 🔵 azul | silêncio | desligado |

As faixas de alarme e as de controle são **independentes** e ficam em `app_config.h`
(`TEMP_IDEAL_MIN/MAX` e `TEMP_ALERTA_BANDA` para o alarme; `HEATER_ON_BELOW_C` e
`HEATER_OFF_ABOVE_C` para o aquecedor).

## Página web e API

A `task_wifi_ap` cria uma rede Wi-Fi (**SoftAP `IncubadoraAP`**) e a `task_http_server` sobe
um servidor em **`http://192.168.4.1`**:

| Rota | Método | Função |
|---|---|---|
| `/` | GET | Página HTML com temperatura, umidade, pressão e estado do aquecedor |
| `/api/status` | GET | JSON com as leituras atuais e o status |
| `/api/config` | GET / POST | Consulta e ajusta as faixas de alarme |
