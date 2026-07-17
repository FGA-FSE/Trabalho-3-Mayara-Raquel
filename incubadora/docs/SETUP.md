# Guia de setup — do zero ao módulo funcionando

Guia autocontido para quem **nunca mexeu com ESP-IDF**. Seguindo do início ao
fim, você compila, grava e vê o módulo de temperatura funcionando.

> Desenvolvido e testado com **ESP-IDF v6.0.2** numa placa **ESP32-WROOM
> (DevKit)**. Versões 5.4+ também devem funcionar (o driver I2C novo
> `i2c_master` existe desde a 5.2).

---

## Seção 1 — Pré-requisitos de software

| Item | Versão mínima | Como verificar |
|---|---|---|
| SO | Linux (Ubuntu 22.04+), macOS ou WSL2 | — |
| Python | 3.8+ | `python3 --version` |
| Git | qualquer recente | `git --version` |
| CMake | 3.16+ | `cmake --version` |
| Ninja | qualquer | `ninja --version` |
| Compilador host | gcc/build-essential | `gcc --version` |

> **Windows nativo não é recomendado** (caminhos de compilação muito longos
> e problemas de driver USB). Use WSL2.

No Ubuntu, as dependências de build:

```bash
sudo apt update
sudo apt install git wget flex bison gperf python3 python3-venv python3-pip \
     cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

**Driver USB-Serial** (CP2102 ou CH340, conforme a placa):

```bash
ls /dev/ttyUSB*   # placas com CP2102/CH340 aparecem aqui
ls /dev/ttyACM*   # placas com USB nativo aparecem aqui
```

Permissão para gravar sem `sudo` (fazer uma vez, depois **relogar**):

```bash
sudo usermod -aG dialout $USER
```

---

## Seção 2 — Instalação do ESP-IDF

Há duas formas. Escolha uma.

### Opção A — Instalador oficial (EIM) — recomendada
Baixe o **ESP-IDF Installation Manager** em
<https://dl.espressif.com/dl/esp-idf/> e siga o assistente (instala a
toolchain e o Python env automaticamente). Ao final, ative o ambiente com o
script gerado, por exemplo:

```bash
. ~/.espressif/tools/activate_idf_v6.0.2.sh
```

### Opção B — Clonar manualmente

```bash
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v6.0.2          # ou a release estável mais recente
./install.sh esp32
. ./export.sh
```

**Dica (vale para as duas opções):** crie um alias para não esquecer de
carregar o ambiente em cada terminal novo:

```bash
# no ~/.bashrc  (ajuste o caminho conforme sua instalação)
alias get_idf='. ~/.espressif/tools/activate_idf_v6.0.2.sh'
```

**Verificação:**
```bash
idf.py --version                # deve exibir a versão do ESP-IDF
xtensa-esp32-elf-gcc --version  # deve exibir o cross-compiler
```

---

## Seção 3 — Montagem do hardware

Barramento **I2C compartilhado** entre sensor e display; LED RGB por PWM.

| Sinal | GPIO | Observação |
|---|---|---|
| I2C SDA (BME280 + OLED) | 21 | barramento compartilhado |
| I2C SCL (BME280 + OLED) | 22 | barramento compartilhado |
| LED R | 25 | LEDC PWM, **resistor em série** |
| LED G | 26 | LEDC PWM, **resistor em série** |
| LED B | 27 | LEDC PWM, **resistor em série** |

**Sensor BME280/BMP280 (I2C):**
- `VCC → 3V3`, `GND → GND`, `SCL → GPIO22`, `SDA → GPIO21`
- `CSB → 3V3` (força modo I2C — **sem isso o sensor fica em SPI e não responde**)
- `SDO → GND` (define endereço **0x76**; em 3V3 seria 0x77)

**Display OLED SSD1306 (I2C):** entra em paralelo no mesmo SDA/SCL.
- `VCC → 3V3`, `GND → GND`, `SCL → GPIO22`, `SDA → GPIO21`
- Endereço típico **0x3C**.

**LED RGB WCMCU 5050 (ânodo comum):**
- Pino comum (`+`) → **3V3**  ⚠️ *este módulo é **ânodo comum*** (verificado
  em bancada). Se usar outro módulo de **catodo comum**, ligue o comum no
  **GND** e troque `LED_COMMON_ANODE` para `false` em `main/app_config.h`.
- `R/G/B → GPIO 25/26/27`, **cada um com resistor de 220–330 Ω em série**
  (o módulo não tem resistores integrados — ligar direto pode danificar o
  GPIO e o LED).

**Alimentação:** não alimente a ESP32 por USB e por VIN externo ao mesmo
tempo sem saber o que está fazendo.

---

## Seção 4 — Clonando e configurando o projeto

```bash
git clone <url-do-repositorio>
cd incubadora

# Configurar o target (uma vez após clonar)
make set-target

# (Opcional) ajustar configurações
make menuconfig
```

---

## Seção 5 — Compilando, gravando e monitorando

Use o **Makefile** de conveniência (`make help` lista tudo):

```bash
make build          # só compila (verifica erros sem precisar da placa)
make flash          # compila + grava (placa conectada via USB)
make monitor        # abre o monitor serial (Ctrl+] para sair)
make fm             # atalho: flash + monitor em sequência

# Se a porta não for /dev/ttyUSB0:
make fm PORT=/dev/ttyACM0
```

---

## Seção 6 — Testando o módulo

Quando estiver funcionando, você deve observar:

1. **Monitor serial**: logs de leitura a cada 2 s (temperatura, pressão, e
   umidade se for um BME280).
2. **Display OLED**: mostra "INCUBADORA", a temperatura grande, pressão (e
   umidade), e o estado (IDEAL/ALERTA/FORA/ERRO).
3. **LED RGB**: cor conforme a faixa (verde ideal, amarelo alerta, vermelho
   fora, azul inicializando/erro).
4. **Wi-Fi**: aparece a rede **`IncubadoraAP`** (senha `incubadora123`).
   Conecte pelo celular/notebook.
5. **Página web**: abra `http://192.168.4.1/` — mostra os dados com
   atualização automática a cada 2 s.
6. **API JSON**: `http://192.168.4.1/api/status` retorna, por exemplo:
   `{"temperature":30.12,"humidity":null,"pressure":1013.2,"status":"ok","sensor_ok":true}`
   (`humidity` vem `null` no BMP280; vira número num BME280).

---

## Seção 7 — Solução de problemas comuns

| Problema | Causa provável | Solução |
|---|---|---|
| `Permission denied` ao gravar | usuário fora do grupo `dialout` | `sudo usermod -aG dialout $USER` + **relogar** |
| `idf.py: command not found` | ambiente ESP-IDF não carregado | rode `get_idf` (ou `. .../activate_idf_*.sh`) |
| `No serial data received` | porta errada / placa desconectada | `ls /dev/tty*`; use `PORT=...` |
| `driver/i2c_master.h: No such file` | `REQUIRES` sem `esp_driver_i2c` | já corrigido nos `CMakeLists.txt` deste projeto |
| Display OLED não acende | SDA/SCL invertidos ou endereço errado | conferir fiação; ver se `0x3C` aparece no scan/monitor |
| Sensor retorna erro / não acha | `CSB` não está em 3V3, ou `SDO` solto | conferir `CSB→3V3` e `SDO→GND` (endereço 0x76) |
| LED RGB não acende | ânodo/catodo trocado, resistor faltando, ou comum solto | módulo é **ânodo comum** (comum → 3V3); conferir resistores; ver `LED_COMMON_ANODE` |
| Erro de flash `Serial noise/corruption` | ruído/cabo USB | regravar; usar `make fm BAUD=115200`; trocar cabo/porta |

Para instalação e primeira execução, este é o guia de referência. O
`README.md` traz uma versão resumida (quick start).
