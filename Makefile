SHELL := /bin/bash

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILDROOT_DIR ?= $(ROOT)/buildroot
EXTERNAL_DIR := $(ROOT)/external
OUTPUT_DIR ?= $(ROOT)/output
DEFCONFIG := linux_embarque_defconfig

.DEFAULT_GOAL := help

.PHONY: help setup defconfig menuconfig linux-menuconfig build rebuild clean distclean legal-info graph-depends check

help:
	@echo "Linux-embarque - cibles disponibles"
	@echo "  setup             Telecharge et prepare Buildroot"
	@echo "  defconfig         Applique $(DEFCONFIG)"
	@echo "  menuconfig        Ouvre la configuration Buildroot"
	@echo "  linux-menuconfig  Ouvre la configuration du noyau"
	@echo "  build             Compile et journalise la construction"
	@echo "  rebuild           Reconstruit sans supprimer dl/"
	@echo "  clean             Nettoyage leger"
	@echo "  distclean         Supprime output/ apres confirmation"
	@echo "  legal-info        Genere les informations de licences"
	@echo "  graph-depends     Genere le graphe des dependances"
	@echo "  check             Lance les controles locaux"

setup:
	@$(ROOT)/scripts/setup.sh

defconfig:
	@$(MAKE) -C $(BUILDROOT_DIR) BR2_EXTERNAL=$(EXTERNAL_DIR) O=$(OUTPUT_DIR) $(DEFCONFIG)

menuconfig: defconfig
	@$(MAKE) -C $(BUILDROOT_DIR) O=$(OUTPUT_DIR) menuconfig

linux-menuconfig: defconfig
	@$(MAKE) -C $(BUILDROOT_DIR) O=$(OUTPUT_DIR) linux-menuconfig

build:
	@$(ROOT)/scripts/build.sh

rebuild:
	@$(ROOT)/scripts/rebuild.sh

clean:
	@$(ROOT)/scripts/clean.sh light

distclean:
	@$(ROOT)/scripts/clean.sh output

legal-info: defconfig
	@$(MAKE) -C $(BUILDROOT_DIR) O=$(OUTPUT_DIR) legal-info

graph-depends: defconfig
	@$(MAKE) -C $(BUILDROOT_DIR) O=$(OUTPUT_DIR) graph-depends

check:
	@if command -v pwsh >/dev/null 2>&1; then \
		pwsh -NoProfile -File $(ROOT)/scripts/verify.ps1; \
	elif command -v powershell.exe >/dev/null 2>&1; then \
		powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$$(wslpath -w $(ROOT))/scripts/verify.ps1"; \
	else \
		echo "PowerShell unavailable; running Bash syntax checks only"; \
		bash -n $(ROOT)/scripts/*.sh $(ROOT)/external/board/linux-embarque/*.sh; \
	fi
