# ---------- Incubadora — módulo de temperatura ----------
# Pré-requisito: ESP-IDF instalado e ambiente carregado no terminal
# (ex.: `get_idf` ou `. $IDF_PATH/export.sh`).
# Uso: make build, make flash, make monitor, make fm, etc.

SHELL     := /bin/bash
PORT      ?= /dev/ttyUSB0
BAUD      ?= 115200

# O EIM expõe o `idf.py` como FUNÇÃO do bash, que não é herdada pelos
# sub-shells que o make usa nas receitas (por isso `idf.py` "não é encontrado"
# dentro do make, mesmo funcionando no terminal). Resolvemos chamando o script
# real: se houver um idf.py executável no PATH usamos ele; senão, invocamos via
# python usando o $IDF_PATH definido por `get_idf`.
IDF := $(shell command -v idf.py 2>/dev/null || echo python $(IDF_PATH)/tools/idf.py)

.DEFAULT_GOAL := help
.PHONY: help build flash monitor fm menuconfig clean fullclean erase size set-target

help:                       ## mostra esta ajuda
	@grep -E '^[a-zA-Z_-]+:.*##' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS=":.*##"}; {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}'

build:                      ## compila o projeto (equivale a idf.py build)
	$(IDF) build

flash:                      ## compila e grava na ESP32 (porta $(PORT))
	$(IDF) -p $(PORT) -b $(BAUD) flash

monitor:                    ## abre o monitor serial (Ctrl+] para sair)
	$(IDF) -p $(PORT) -b $(BAUD) monitor

fm: flash monitor           ## flash + monitor em sequência (atalho mais usado)

menuconfig:                 ## abre o menu de configuração (Kconfig)
	$(IDF) menuconfig

clean:                      ## limpa artefatos de build (mantém sdkconfig)
	$(IDF) clean

fullclean:                  ## limpa tudo, incluindo sdkconfig gerado
	$(IDF) fullclean

erase:                      ## apaga toda a flash da ESP32
	$(IDF) -p $(PORT) erase-flash

size:                       ## mostra uso de memória (RAM/flash) por componente
	$(IDF) size-components

set-target:                 ## configura target como esp32 (rodar uma vez após clonar)
	$(IDF) set-target esp32
