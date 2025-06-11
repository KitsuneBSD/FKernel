section .multiboot_header align=8

header_start:
  dd 0xE85250D6                ; magic
  dd 0                         ; architecture
  dd header_end - header_start ; header length (em bytes)
  dd -(0xE85250D6 + 0 + (header_end - header_start)) ; checksum

  dw 0                        ; tag type = end (0)
  dw 0                        ; flags
  dd 8                        ; size do tag end (8 bytes)

header_end:
