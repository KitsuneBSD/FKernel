include ../../../../config.mk

.PHONY: all clean

BUILD_DIR=../../../../Build
OBJ_DIR=$(BUILD_DIR)/Obj
INCLUDE_DIR=../../../../Include

ASM_FILES=$(wildcard *.asm)
ASM_OBJS=$(patsubst %.asm, $(OBJ_DIR)/%.o, $(ASM_FILES))

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

clean:
	@rm -rf $(ASM_OBJS) $(C_OBJS)

$(BUILD_DIR):
	@echo "Create Build Directory"
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@echo "Create Build/Obj Directory"
	@mkdir -p $(OBJ_DIR)

