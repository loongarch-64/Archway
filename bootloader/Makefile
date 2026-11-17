# SPDX-License-Identifier: MulanPSL-2.0

PICOEFI_DIR   := ../third_party/PicoEFI
PICOEFI_STAMP := $(PICOEFI_DIR)/.downloaded
FIRMWARE      := ../firmware/QEMU_EFI.fd

BUILD_DIR := build
EFI_FILE  := $(BUILD_DIR)/Archway.efi
EFI_BOOT  := $(BUILD_DIR)/esp/EFI/BOOT/BOOTLOONGARCH64.EFI
UEFI_VARS := $(BUILD_DIR)/QEMU_VARS.fd
UEFI_VARS_TEMPLATE := no-timeout-vars/QEMU_VARS.fd.gz
SOURCES := $(wildcard src/*.c src/*.S)
PICOEFI_C_SOURCES := $(wildcard $(PICOEFI_DIR)/src/loongarch64/*.c)
PICOEFI_ASM_SOURCES := $(wildcard $(PICOEFI_DIR)/src/loongarch64/*.S)
PICOEFI_C_OBJECTS := $(patsubst $(PICOEFI_DIR)/src/loongarch64/%.c,$(BUILD_DIR)/%.o,$(PICOEFI_C_SOURCES))
PICOEFI_ASM_OBJECTS := $(patsubst $(PICOEFI_DIR)/src/loongarch64/%.S,$(BUILD_DIR)/%.o,$(PICOEFI_ASM_SOURCES))
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst src/%.S,$(BUILD_DIR)/%.o,$(filter-out src/handoff.S,$(filter %.S,$(SOURCES)))) \
	$(BUILD_DIR)/transition.o $(PICOEFI_C_OBJECTS) $(PICOEFI_ASM_OBJECTS)

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
	-I$(PICOEFI_DIR)/include \
	-I../include

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
.PHONY: kernel

.DELETE_ON_ERROR:

all: $(EFI_FILE)

-include $(BUILD_DIR)/*.d

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

$(OBJECTS): $(BUILD_DIR)/compile.config

kernel:
	$(MAKE) -C ../archway all

../archway/build/archway.elf: kernel
	@test -f $@

$(BUILD_DIR)/%.o: src/%.c Makefile $(PICOEFI_STAMP)
	mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) $(if $(filter string.o,$(@F)),-fno-lto) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: src/%.S Makefile
	mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/transition.o: src/handoff.S Makefile
	mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(PICOEFI_C_OBJECTS): $(BUILD_DIR)/%.o: $(PICOEFI_DIR)/src/loongarch64/%.c $(PICOEFI_STAMP) Makefile
	mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(PICOEFI_ASM_OBJECTS): $(BUILD_DIR)/%.o: $(PICOEFI_DIR)/src/loongarch64/%.S $(PICOEFI_STAMP) Makefile
	mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/Archway.elf: $(BUILD_DIR)/link.config $(OBJECTS) $(PICOEFI_DIR)/src/loongarch64/link_script.lds Makefile
	$(CC) $(EFI_LD_DRIVER_FLAGS) $(EFI_CFLAGS) $(EFI_LDFLAGS) $(OBJECTS) -o $@

$(EFI_FILE): $(BUILD_DIR)/Archway.elf $(BUILD_DIR)/copy.config
	$(OBJCOPY) -O binary $< $@

$(EFI_BOOT): $(EFI_FILE)
	mkdir -p $(dir $@)
	cp $< $@

$(BUILD_DIR)/esp/archway.elf: ../archway/build/archway.elf
	mkdir -p $(dir $@)
	cp $< $@

# Loongson QEMU_VARS.fd with the UEFI global Timeout variable set to zero.
$(UEFI_VARS): $(UEFI_VARS_TEMPLATE)
	mkdir -p $(dir $@)
	gzip -dc $< > $@.tmp
	mv $@.tmp $@

run: $(EFI_BOOT) $(BUILD_DIR)/esp/archway.elf $(UEFI_VARS)
	test -f $(FIRMWARE) || { echo "Run 'make download-required' from the repository root first."; exit 1; }
	qemu-system-loongarch64 -m 2G -M virt,pflash0=pflash0,pflash1=pflash1 -cpu la464 \
		-drive if=none,id=pflash0,format=raw,readonly=on,file=$(FIRMWARE) \
		-drive if=none,id=pflash1,format=raw,file=$(UEFI_VARS) \
		-serial stdio -device ramfb -drive file=fat:rw:$(BUILD_DIR)/esp

clean:
	rm -rf $(BUILD_DIR)
