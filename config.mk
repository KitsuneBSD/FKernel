LD=ld.lld
LD_FLAGS=-nostdlib -T $(CONFIG_DIR)/Linker.ld

CC=clang 

CFLAGS=-ffreestanding -nostdlib -nostdinc -O2 -c -fno-stack-protector 
CFLAGS += -I $(INCLUDE_DIR)/Kernel/Driver -I $(INCLUDE_DIR)/Kernel/Descriptor -I $(INCLUDE_DIR)/LibK

AS=nasm
ASFLAGS=-felf64 -O2

