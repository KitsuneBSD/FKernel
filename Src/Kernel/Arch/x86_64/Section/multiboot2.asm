%define MULTIBOOT2_MAGIC 0xe85250d6
%define MULTIBOOT2_i386_ARCHITECTURE 0

section .multiboot_header
align 8
header_start:
  dd MULTIBOOT2_MAGIC
  dd MULTIBOOT2_i386_ARCHITECTURE
  dd header_end - header_start
  dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_i386_ARCHITECTURE + (header_end - header_start))
  ; End tag obrigatório
  dw 0
  dw 0
  dd 8
  ; TODO: Validate that this header is placed within the first 32KiB of
  ; the final binary image. Some tools (grub-file) require the header to be
  ; reachable within the first 32KiB of the file image to be recognized.
  ; FIXME: If the linker places this section in a non-loadable segment or
  ; at an offset that gurub doesn't inspect, GRUB may not detect it.
header_end:
