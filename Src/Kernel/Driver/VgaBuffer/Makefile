.PHONY: all clean

BUILD_DIR=../../../../Build
OBJ_DIR=$(BUILD_DIR)/Obj
INCLUDE_DIR=../../../../Include

AS=nasm
ASFLAGS=-felf64 -O2

ASM_FILES=$(wildcard *.asm)
ASM_OBJS=$(patsubst %.asm, $(OBJ_DIR)/%.o, $(ASM_FILES))

CC=clang 
CFLAGS=-ffreestanding -nostdlib -nostdinc -O2 -c -I $(INCLUDE_DIR)/Kernel/Driver -I $(INCLUDE_DIR)/LibK

C_FILES=$(wildcard *c)
C_OBJS=$(patsubst %.c, $(OBJ_DIR)/%.o, $(C_FILES))

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.asm | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

all: $(ASM_OBJS) $(C_OBJS)

clean: $(ASM_OBJS) $(C_OBJS)
	@echo "Cleaning Src/Kernel/Driver/VgaBuffer Objects"
	@echo "RM: $<"
	@rm -rf $(ASM_OBJS) $(C_OBJS)

$(BUILD_DIR):
	@echo "Create Build Directory"
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@echo "Create Build/Obj Directory"
	@mkdir -p $(OBJ_DIR)

