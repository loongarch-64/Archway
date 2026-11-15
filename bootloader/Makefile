# SPDX-License-Identifier: MulanPSL-2.0

PICOEFI_DIR   := ../third_party/PicoEFI
PICOEFI_STAMP := $(PICOEFI_DIR)/.downloaded
FIRMWARE      := ../firmware/QEMU_EFI.fd

BUILD_DIR := build
EFI_FILE  := $(BUILD_DIR)/Archway.efi
EFI_BOOT  := $(BUILD_DIR)/esp/EFI/BOOT/BOOTLOONGARCH64.EFI

CLANG        ?= clang
LLD          ?= lld
LLVM_OBJCOPY ?= llvm-objcopy
CC           := $(CLANG) --target=loongarch64-unknown-none-elf
OBJCOPY      := $(LLVM_OBJCOPY)
EFI_LD_DRIVER_FLAGS := -fuse-ld=$(LLD)

EFI_CFLAGS := \
	-O2 \
	-Wall -Wextra \
	-std=gnu11 \
	-ffreestanding \
	-fno-common \
	-fno-stack-protector \
	-fno-stack-check \
	-fshort-wchar \
	-fPIE \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	-mabi=lp64s \
	-march=loongarch64 \
	-msoft-float \
	-I$(PICOEFI_DIR)/include

EFI_LDFLAGS := \
	-nostdlib \
	-Wl,-pie \
	-Wl,-z,text \
	-Wl,-z,max-page-size=0x1000 \
	-Wl,-z,noexecstack \
	-Wl,--gc-sections \
	-Wl,--build-id=none \
	-Wl,--hash-style=gnu \
	-Wl,-m,elf64loongarch \
	-Wl,-T,$(PICOEFI_DIR)/src/loongarch64/link_script.lds

.PHONY: all run clean

all: $(EFI_FILE)

$(BUILD_DIR)/main.o: src/main.c $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -c $< -o $@

$(BUILD_DIR)/entry.o: $(PICOEFI_DIR)/src/loongarch64/entry.S $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -c $< -o $@

$(BUILD_DIR)/reloc.o: $(PICOEFI_DIR)/src/loongarch64/reloc.c $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -c $< -o $@

$(BUILD_DIR)/Archway.elf: $(BUILD_DIR)/main.o $(BUILD_DIR)/entry.o $(BUILD_DIR)/reloc.o $(PICOEFI_DIR)/src/loongarch64/link_script.lds Makefile
	$(CC) $(EFI_LD_DRIVER_FLAGS) $(EFI_CFLAGS) $(EFI_LDFLAGS) $(BUILD_DIR)/main.o $(BUILD_DIR)/entry.o $(BUILD_DIR)/reloc.o -o $@

$(EFI_FILE): $(BUILD_DIR)/Archway.elf
	$(OBJCOPY) -O binary $< $@

$(EFI_BOOT): $(EFI_FILE)
	mkdir -p $(dir $@)
	cp $< $@

run: $(EFI_BOOT)
	test -f $(FIRMWARE) || { echo "Run 'make download-required' from the repository root first."; exit 1; }
	qemu-system-loongarch64 -m 2G -M virt -cpu la464 -bios $(FIRMWARE) -serial stdio -device ramfb -drive file=fat:rw:$(BUILD_DIR)/esp

clean:
	rm -rf $(BUILD_DIR)
