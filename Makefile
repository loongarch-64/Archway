FIRMWARE_DIR := firmware
FIRMWARE     := $(FIRMWARE_DIR)/QEMU_EFI.fd
FIRMWARE_URL := https://raw.githubusercontent.com/loongson/Firmware/main/LoongArchVirtMachine/QEMU_EFI.fd

.PHONY: download-required

download-required: $(FIRMWARE)

$(FIRMWARE):
	mkdir -p $(FIRMWARE_DIR)
	curl -fL $(FIRMWARE_URL) -o $@.tmp
	mv $@.tmp $@
