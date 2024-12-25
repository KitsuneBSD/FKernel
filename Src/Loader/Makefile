BUILD_DIR=../../Build/Loader
OBJ_DIR=$(BUILD_DIR)/Obj

ASM_FILES=$(wildcard *.asm)
ASM_OBJS=$(patsubst %.asm,$(OBJ_DIR)/%.bin,$(ASM_FILES))

all: $(BUILD_DIR)/boot.bin

clean:
	rm -rf $(BUILD_DIR)

$(OBJ_DIR)/%.bin: %.asm
	@mkdir -p $(OBJ_DIR)
	nasm -f bin $< -o $@

$(BUILD_DIR)/boot.bin: $(ASM_OBJS)
	cat $^ > $(BUILD_DIR)/boot.bin
