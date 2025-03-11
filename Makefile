.PHONY: all build run clean compile iso

include ./config.mk

BUILD_DIR=Build
OBJ_DIR=$(BUILD_DIR)/Obj
CONFIG_DIR=Config

FKernelOS=$(BUILD_DIR)/FKernelOS
ISO_PATH=$(BUILD_DIR)/FKernel_MockOS.iso

KERNEL_BIN=$(BUILD_DIR)/kernel.bin
OBJS=$(shell find $(OBJ_DIR) -type f -name "*.o")

all: build run

build: $(KERNEL_BIN)

$(KERNEL_BIN): compile
	@echo "Linking kernel..."
	@$(LD) $(LD_FLAGS) $(OBJS) -o $@

compile:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)
	@find Src/Kernel/* -type f -name Makefile -execdir $(MAKE) -s -C $(dir {}) \;

iso: build
	@echo "Creating MockOS ISO"
	@mkdir -p $(FKernelOS)/boot/grub
	@cp $(CONFIG_DIR)/grub.cfg $(FKernelOS)/boot/grub
	@cp $(KERNEL_BIN) $(FKernelOS)/boot/
	@grub-mkrescue -o $(ISO_PATH) $(FKernelOS) > /dev/null 2>&1
	@rm -rf $(FKernelOS)

run: iso 
	@echo "Running MockOS in QEMU"
	@qemu-system-x86_64 -cdrom $(ISO_PATH)

clean:
	@echo "Clean build..."
	@find Src/Kernel/* -type f -name Makefile -execdir $(MAKE) -s -C $(dir {}) clean \;
	@rm -rf $(BUILD_DIR)


	
