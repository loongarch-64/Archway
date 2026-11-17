# SPDX-License-Identifier: MulanPSL-2.0

CLANG ?= clang
LLD ?= lld
BUILD_DIR := build
KERNEL := $(BUILD_DIR)/archway.elf
LINKER_SCRIPT := $(BUILD_DIR)/linker.ld
SOURCES := $(shell find src -type f \( -name '*.c' -o -name '*.S' -o -name '*.s' \))
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst src/%.S,$(BUILD_DIR)/%.S.o,$(filter %.S,$(SOURCES))) \
	$(patsubst src/%.s,$(BUILD_DIR)/%.s.o,$(filter %.s,$(SOURCES)))
CC := $(CLANG) --target=loongarch64-unknown-none-elf
KERNEL_CFLAGS := \
	-O2 -Wall -Wextra -std=gnu11 \
	-ffreestanding -fno-builtin -fno-common -fno-stack-protector -fno-stack-check \
	-fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables \
	-mabi=lp64s -march=loongarch64 -msoft-float \
	-I../include -Isrc

.PHONY: all clean
.DELETE_ON_ERROR:
all: $(KERNEL)

-include $(OBJECTS:.o=.d)

$(BUILD_DIR)/%.S.o: src/%.S Makefile
	mkdir -p $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.s.o: src/%.s Makefile
	mkdir -p $(dir $@)
	$(CC) -march=loongarch64 -mabi=lp64s -c $< -o $@

$(BUILD_DIR)/%.o: src/%.c Makefile
	mkdir -p $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(LINKER_SCRIPT): linker.ld Makefile
	mkdir -p $(dir $@)
	$(CLANG) -E -P -x c $< -o $@

$(KERNEL): $(OBJECTS) $(LINKER_SCRIPT) Makefile
	$(CC) -fuse-ld=$(LLD) -nostdlib -static -Wl,-m,elf64loongarch -Wl,--build-id=none -Wl,--orphan-handling=error -Wl,-z,max-page-size=0x1000 -Wl,-T,$(LINKER_SCRIPT) $(OBJECTS) -o $@

clean:
	rm -rf $(BUILD_DIR)
