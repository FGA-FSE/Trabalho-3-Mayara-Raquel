# Incubadora Inteligente com ESP32

**Universidade de Brasília (UnB) — Faculdade do Gama (FGA)**
**Disciplina:** Fundamentos de Sistemas Embarcados

**Integrantes:**

| Integrante | Matrícula |
|---|---|
| Mayara Alves de Oliveira | 200025058 |
| Raquel Temóteo Eucaria Pereira da Costa | 202045268 |

---

## Sobre o projeto

<p align="center">
  <img src="docs/imagens/incubadora.png" alt="Incubadora ligada à ESP32" width="480">
</p>

Este projeto é o firmware de uma **incubadora inteligente** desenvolvida em **ESP32** com o
framework **ESP-IDF**. O sistema monitora a **temperatura** e a **umidade** dentro da câmara,
mantém a temperatura na faixa ideal controlando um **aquecedor** por meio de um **relé**, e
avisa quando algo sai do esperado através de um **LED RGB** e de um **buzzer**. As leituras
podem ser acompanhadas tanto em um **display OLED** quanto em uma **página web** servida pela
própria ESP32 via Wi-Fi.

Todo o firmware roda sobre o **FreeRTOS**, com cada responsabilidade em uma tarefa
independente, sincronizadas por um estado compartilhado protegido por *mutex*.

## Objetivo

- Construir um protótipo funcional de incubadora usando a plataforma ESP32 e o ESP-IDF;
- Medir temperatura, pressão e umidade com sensores de baixo custo;
- Controlar automaticamente o aquecedor mantendo a temperatura em uma faixa estreita;
- Sinalizar o estado do sistema de forma visual (LED), sonora (buzzer) e remota (página web);
- Organizar o código de forma modular, com drivers reutilizáveis e tarefas desacopladas.

## Como funciona

- **Temperatura e pressão** são lidas pelo sensor **BMP280** (I²C);
- **Umidade** é lida pelo sensor **DHT11**;
- O **aquecedor** (relé) usa **histerese**: liga quando a temperatura cai a **28 °C** e
  desliga quando atinge **29 °C**;
- O **LED RGB** fica **verde** na faixa segura (**27 a 32 °C**) e **vermelho** fora dela;
- O **buzzer** apita quando a temperatura sai da faixa segura;
- O **OLED** e a **página web** mostram as leituras e o estado do aquecedor.

> A documentação completa (componentes, funções e arquitetura) está em [`docs/`](docs/) e
> publicada em: **https://fga-fse.github.io/Trabalho-3-Mayara-Raquel/**

## Componentes e ligação na ESP

Todos os pinos ficam centralizados em [`incubadora/main/app_config.h`](incubadora/main/app_config.h).

| Componente | Pino (ESP32) | Observação |
|---|---|---|
| BMP280 + OLED — **SDA** | GPIO21 | barramento I²C compartilhado |
| BMP280 + OLED — **SCL** | GPIO22 | barramento I²C compartilhado |
| DHT11 (dados) | GPIO32 | sensor de umidade (1 fio) |
| LED RGB (R / G / B) | GPIO25 / 26 / 27 | PWM, ânodo comum |
| Buzzer | GPIO33 | ativo em nível alto |
| Relé (aquecedor) | GPIO19 | ativo em nível baixo |

## Como rodar (compilar e gravar na ESP)

### Pré-requisitos

- **ESP-IDF** instalado (versão 6.x);
- uma placa **ESP32** com os componentes ligados conforme a tabela acima.

### Passos

O projeto do firmware fica na pasta [`incubadora/`](incubadora/). Com o ambiente do ESP-IDF
ativado, execute:

```bash
cd incubadora

# compila o firmware
idf.py build

# grava na placa e abre o monitor serial (troque COM5 pela porta da sua placa)
idf.py -p COM5 flash monitor
```

> Para sair do monitor serial, pressione **Ctrl + ]**.
> O alvo (`esp32`) já vem configurado no `sdkconfig`.

Após a gravação, a placa reinicia e o firmware começa a rodar automaticamente: os sensores
passam a ser lidos, o display mostra as leituras e o controle do aquecedor entra em ação.

## Acessando a página web

A ESP32 cria a sua **própria rede Wi-Fi** (não precisa de roteador). Para abrir a página:

1. No celular ou no computador, conecte-se à rede Wi-Fi **`IncubadoraAP`**
   (senha: **`incubadora123`**);
2. Abra o navegador e acesse **http://192.168.4.1**;
3. A página mostra **temperatura**, **umidade**, **pressão** e o **estado do aquecedor**,
   atualizando sozinha. Também é possível **ajustar as faixas de alarme** por ali.

## 🎥 Vídeo de apresentação

Apresentação do projeto em funcionamento:

**▶️ https://youtu.be/D7ZEasUPcvo**
