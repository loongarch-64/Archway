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

.PHONY: all run clean FORCE

.DELETE_ON_ERROR:

all: $(EFI_FILE)

-include $(BUILD_DIR)/*.d

# Record effective commands, preserving timestamps when the configuration is unchanged.
# Quote single quotes in overrides before passing them to the shell.
quote = '$(subst ','"'"',$(1))'
COMPILE_COMMAND := $(CC) $(EFI_CFLAGS)
LINK_COMMAND := $(CC) $(EFI_LD_DRIVER_FLAGS) $(EFI_CFLAGS) $(EFI_LDFLAGS)
COPY_COMMAND := $(OBJCOPY)

FORCE:

$(BUILD_DIR)/compile.config: FORCE
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' $(call quote,$(COMPILE_COMMAND)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(BUILD_DIR)/link.config: FORCE
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' $(call quote,$(LINK_COMMAND)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(BUILD_DIR)/copy.config: FORCE
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' $(call quote,$(COPY_COMMAND)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(BUILD_DIR)/main.o $(BUILD_DIR)/mm.o $(BUILD_DIR)/strhlp.o $(BUILD_DIR)/string.o $(BUILD_DIR)/entry.o $(BUILD_DIR)/reloc.o: $(BUILD_DIR)/compile.config

$(BUILD_DIR)/main.o: src/main.c $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/mm.o: src/mm.c src/strhlp.h $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/strhlp.o: src/strhlp.c src/strhlp.h Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

# Compiler-emitted memory calls must resolve to these routines even with LTO.
$(BUILD_DIR)/string.o: src/string.c src/string.h Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -fno-lto -MMD -MP -c $< -o $@

$(BUILD_DIR)/entry.o: $(PICOEFI_DIR)/src/loongarch64/entry.S $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/reloc.o: $(PICOEFI_DIR)/src/loongarch64/reloc.c $(PICOEFI_STAMP) Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/Archway.elf: $(BUILD_DIR)/link.config $(BUILD_DIR)/main.o $(BUILD_DIR)/mm.o $(BUILD_DIR)/strhlp.o $(BUILD_DIR)/string.o $(BUILD_DIR)/entry.o $(BUILD_DIR)/reloc.o $(PICOEFI_DIR)/src/loongarch64/link_script.lds Makefile
	$(CC) $(EFI_LD_DRIVER_FLAGS) $(EFI_CFLAGS) $(EFI_LDFLAGS) $(BUILD_DIR)/main.o $(BUILD_DIR)/mm.o $(BUILD_DIR)/strhlp.o $(BUILD_DIR)/string.o $(BUILD_DIR)/entry.o $(BUILD_DIR)/reloc.o -o $@

$(EFI_FILE): $(BUILD_DIR)/Archway.elf $(BUILD_DIR)/copy.config
	$(OBJCOPY) -O binary $< $@

$(EFI_BOOT): $(EFI_FILE)
	mkdir -p $(dir $@)
	cp $< $@

run: $(EFI_BOOT)
	test -f $(FIRMWARE) || { echo "Run 'make download-required' from the repository root first."; exit 1; }
	qemu-system-loongarch64 -m 2G -M virt -cpu la464 -bios $(FIRMWARE) -serial stdio -device ramfb -drive file=fat:rw:$(BUILD_DIR)/esp

clean:
	rm -rf $(BUILD_DIR)
