# SPDX-License-Identifier: MulanPSL-2.0

CLANG ?= clang
LLD ?= lld
BUILD_DIR := build
KERNEL := $(BUILD_DIR)/archway.elf
OBJECTS := $(BUILD_DIR)/entry.o $(BUILD_DIR)/main.o $(BUILD_DIR)/serial.o
CC := $(CLANG) --target=loongarch64-unknown-none-elf
KERNEL_CFLAGS := \
	-O2 -Wall -Wextra -std=gnu11 \
	-ffreestanding -fno-common -fno-stack-protector -fno-stack-check \
	-fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables \
	-mabi=lp64s -march=loongarch64 -msoft-float \
	-I../include

.PHONY: all clean
.DELETE_ON_ERROR:
all: $(KERNEL)

-include $(BUILD_DIR)/*.d

$(BUILD_DIR)/entry.o: src/entry.S Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: src/%.c Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(KERNEL): $(OBJECTS) linker.ld Makefile
	$(CC) -fuse-ld=$(LLD) -nostdlib -static -Wl,-m,elf64loongarch -Wl,--build-id=none -Wl,--orphan-handling=error -Wl,-z,max-page-size=0x1000 -Wl,-T,linker.ld $(OBJECTS) -o $@

clean:
	rm -rf $(BUILD_DIR)
