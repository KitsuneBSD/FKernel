.PHONY: all compile build_os run_os clean

# -- Project Directory
BUILD_DIR=Build
OBJECT_DIR=$(BUILD_DIR)/Obj
FKERNELOS=$(BUILD_DIR)/FKernelOS
FKERNELOS_ISO=$(BUILD_DIR)/FKernelDistro.iso
CONFIG_DIR=Config

TARGET=$(BUILD_DIR)/kernel.bin
OBJECTS=$(wildcard $(OBJECT_DIR)/*.o)

# -- Linker toolchain
LD=ld.lld
LDFLAGS=-nostdlib -T $(CONFIG_DIR)/Linker.ld

all: compile $(TARGET) $(OBJECTS)

build_os: $(TARGET)
	@echo "===> Create OS Mock"
	@mkdir -p $(FKERNELOS)/boot/grub/
	@cp -r $(CONFIG_DIR)/grub.cfg $(FKERNELOS)/boot/grub/grub.cfg
	@cp -r $(BUILD_DIR)/kernel.bin $(FKERNELOS)/boot/
	@grub-mkrescue -o $(BUILD_DIR)/FKernelDistro.iso $(FKERNELOS)
	@rm -rf $(FKERNELOS)

run_os: build_os
	@echo "===> Running OS Mock"
	@qemu-system-x86_64 -m 4G -smp 2 -cdrom $(FKERNELOS_ISO)

compile:
	@$(MAKE) -C Src/Kernel/Boot

clean:
	@$(MAKE) -C Src/Kernel/Boot clean

$(TARGET): $(OBJECTS)
	@echo "Linking objects"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@


