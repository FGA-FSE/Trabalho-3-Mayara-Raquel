# Funções

O firmware é dividido em **tarefas do FreeRTOS** — pequenos programas que rodam "ao mesmo
tempo", cada um cuidando de uma única responsabilidade. Essa divisão deixa o código simples:
em vez de um laço gigante fazendo tudo, cada tarefa tem um objetivo claro e um ritmo próprio.

As tarefas **não conversam diretamente** entre si. Existem tarefas que **produzem** dados
(leem os sensores, decidem o aquecedor) e tarefas que **consomem** esses dados (acendem o LED,
apitam, desenham no display, respondem à web). A ponte entre elas é sempre o **estado
compartilhado** (ver [Arquitetura](../03-analise-tecnica/arquitetura.md)).

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

??? info "Detalhe técnico — tarefas, prioridades e leitura dos sensores"
    Cada tarefa é criada no `app_main()` com `xTaskCreate`, recebendo uma **prioridade** e um
    tamanho de **pilha**. As tarefas de aquisição e controle (`task_sensor`, `task_dht11`,
    `task_control`) têm prioridade um pouco maior que as de saída (display, LED, buzzer),
    porque medir e agir sobre o aquecedor é mais importante que atualizar a tela.

    Os **períodos** vêm de `vTaskDelay`: o sensor e a umidade a cada 2 s, o controle a cada
    1 s (para reagir rápido), o LED a cada 0,3 s e o buzzer a cada 0,25 s (para o apito
    intermitente). A `task_sensor` lê **só temperatura e pressão** (BMP280) e a `task_dht11`
    lê **só umidade** (DHT11) — cada uma publica a sua parte no estado compartilhado.

## Controle do aquecedor (histerese)

A `task_control` é o coração do sistema: ela mantém a temperatura numa faixa estreita
**ligando e desligando o relé** do aquecedor. Não é um liga/desliga simples — usa
**histerese** (uma "banda morta") para o aquecedor não ficar chaveando toda hora, o que
desgastaria o relé e faria a temperatura oscilar demais.

- **Liga** o aquecedor quando a temperatura **cai a 28 °C**;
- **Desliga** quando a temperatura **atinge 29 °C**;
- **entre 28 e 29 °C**, mantém o estado atual.

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
    Se a leitura de temperatura **falhar** (sensor com problema), a `task_control`
    **desliga o aquecedor** por segurança, em vez de continuar aquecendo às cegas.

??? info "Detalhe técnico — por que histerese"
    Sem banda morta, um único limite (ex.: "ligar/desligar em 28,5 °C") faria o relé chavear
    dezenas de vezes por minuto perto do ponto de corte, pois a temperatura sempre flutua
    alguns centésimos. A histerese cria dois limites diferentes (**28** para ligar, **29**
    para desligar): o aquecedor só volta a ligar depois que a temperatura realmente caiu,
    resultando em ciclos longos e estáveis. Os dois valores são `#define` em `app_config.h`
    (`HEATER_ON_BELOW_C` e `HEATER_OFF_ABOVE_C`), fáceis de ajustar.

## Alarme e sinalização

O **LED RGB** e o **buzzer** avisam se a temperatura saiu da **faixa segura (27 a 32 °C)**.
Repare que essa faixa é **mais larga** que a de controle (28–29): o controle mantém a
temperatura no ponto ideal, e o alarme só dispara se algo der muito errado.

| Situação | Temperatura | LED | Buzzer | Aquecedor |
|---|---|---|---|---|
| Muito frio | **< 27 °C** | 🔴 vermelho | apitando | ligado |
| Faixa segura | **27 – 32 °C** | 🟢 verde | silêncio | histerese 28 ↔ 29 °C |
| Muito quente | **> 32 °C** | 🔴 vermelho | apitando | desligado |
| Inicializando / falha de sensor | — | 🔵 azul | silêncio | desligado |

??? info "Detalhe técnico — como o estado é calculado"
    Uma função central, `compute_status()`, traduz a temperatura em um estado
    (`ST_OK`, `ST_WARNING`, `ST_ALARM`, `ST_ERROR`, `ST_INIT`) comparando a leitura com a
    faixa ideal (`TEMP_IDEAL_MIN`/`TEMP_IDEAL_MAX`) mais uma margem (`TEMP_ALERTA_BANDA`).
    Com os valores atuais (28, 31 e 1 °C), o **alarme** (`ST_ALARM`) ocorre abaixo de **27**
    ou acima de **32 °C**. O LED fica **verde** em toda a faixa segura e **vermelho** no
    alarme; o buzzer apita apenas em `ST_ALARM`. As faixas de **alarme** e de **controle** são
    **independentes** — mexer em uma não afeta a outra.

## Página web e API

A `task_wifi_ap` cria uma rede Wi-Fi própria (**SoftAP `IncubadoraAP`**, sem precisar de
roteador) e a `task_http_server` sobe um servidor em **`http://192.168.4.1`**. Ao conectar o
celular nessa rede, a página mostra as leituras atualizando sozinhas.

| Rota | Método | Função |
|---|---|---|
| `/` | GET | Página HTML com temperatura, umidade, pressão e estado do aquecedor |
| `/api/status` | GET | JSON com as leituras atuais e o status |
| `/api/config` | GET / POST | Consulta e ajusta as faixas de alarme |

??? info "Detalhe técnico — servidor e API"
    O servidor usa o `esp_http_server` do ESP-IDF, que roda na sua própria tarefa interna. A
    página em `/` é servida como uma string HTML única (com o JavaScript embutido) que, a cada
    2 s, busca `/api/status` e atualiza os campos. O `/api/status` responde um JSON como
    `{"temperature":28.5,"humidity":41,"pressure":898,"heater":true,"status":"ok"}`. O
    `POST /api/config` recebe novas faixas de alarme (formato *form-encoded*), valida e aplica
    na hora, sem reiniciar.
