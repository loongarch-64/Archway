# SPDX-License-Identifier: MulanPSL-2.0

FIRMWARE_DIR := firmware
FIRMWARE     := $(FIRMWARE_DIR)/QEMU_EFI.fd
FIRMWARE_URL := https://raw.githubusercontent.com/loongson/Firmware/main/LoongArchVirtMachine/QEMU_EFI.fd

PICOEFI_VERSION := 9065da08260729e932531110a5a7fb90c0022aa9
PICOEFI_DIR     := third_party/PicoEFI
PICOEFI_ARCHIVE := third_party/PicoEFI-$(PICOEFI_VERSION).tar.gz
PICOEFI_URL     := https://codeload.github.com/PicoEFI/PicoEFI/tar.gz/$(PICOEFI_VERSION)
PICOEFI_STAMP   := $(PICOEFI_DIR)/.downloaded

.PHONY: all download-required run clean

all: $(PICOEFI_STAMP)
	$(MAKE) -C bootloader all

download-required: $(FIRMWARE) $(PICOEFI_STAMP)

$(FIRMWARE):
	mkdir -p $(FIRMWARE_DIR)
	curl -fL $(FIRMWARE_URL) -o $@.tmp
	mv $@.tmp $@

$(PICOEFI_ARCHIVE):
	mkdir -p third_party
	curl -fL $(PICOEFI_URL) -o $@.tmp
	mv $@.tmp $@

$(PICOEFI_STAMP): $(PICOEFI_ARCHIVE)
	mkdir -p $(PICOEFI_DIR)
	tar -xzf $(PICOEFI_ARCHIVE) --strip-components=1 -C $(PICOEFI_DIR)
	touch $@

run: download-required
	$(MAKE) -C bootloader run

clean:
	$(MAKE) -C bootloader clean
