global error

section .prekernel.text
bits 32

; @brief Display an error message and halt the system
error:
    mov dword [0xb8000], 0x4f524f45  ; "ERROR;"
    mov dword [0xb8004], 0x4f3a4f52  ; "O:R"
    mov dword [0xb8008], 0x4f204f20  ; "O O "
    mov byte  [0xb800a], al          ; código do erro (M ou L)
.halt:
    hlt
    jmp .halt
