BUILD_DIR=../../Build/Loader

ASM_FILES=$(wildcard *.asm)
ASM_OBJS=$(patsubst %.asm,$(BUILD_DIR)/%.bin,$(ASM_FILES))

all: $(ASM_OBJS)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/%.bin: %.asm
	@mkdir -p $(BUILD_DIR)
	nasm -f bin $< -o $@
