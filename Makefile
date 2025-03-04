.PHONY: all compile clean

# -- Project Directory
BUILD_DIR=Build
OBJECT_DIR=$(BUILD_DIR)/Obj
CONFIG_DIR=Config

TARGET=$(BUILD_DIR)/kernel.bin
OBJECTS=$(wildcard $(OBJECT_DIR)/*.o)

# -- Linker toolchain
LD=ld.lld
LDFLAGS=-nostdlib -T $(CONFIG_DIR)/Linker.ld

# NOTE: Maybe be a good idea in future change the default Makefile to KConfig
all: compile $(TARGET) $(OBJECTS)

compile:
	@$(MAKE) -C Src/Kernel/Boot

clean:
	@$(MAKE) -C Src/Kernel/Boot clean

$(TARGET): $(OBJECTS)
	@echo "Linking objects"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@


