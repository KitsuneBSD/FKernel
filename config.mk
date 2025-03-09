INCLUDE_FLAGS := $(patsubst %,-I%,$(shell find $(INCLUDE_DIR) -type d))

LD=ld.lld
LD_FLAGS=-nostdlib -T $(CONFIG_DIR)/Linker.ld

CC=clang 

CFLAGS=-ffreestanding -nostdlib -nostdinc -O2 -c
CFLAGS += $(INCLUDE_FLAGS) -fno-stack-protector -fno-strict-aliasing
CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls

AS=nasm
ASFLAGS=-felf64 -O2

