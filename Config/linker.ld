ENTRY(start)

SECTIONS
{
    . = 1M;

    .kernel : 
    {
        *(.multiboot_header)  /* Mantém o header multiboot aqui */

        *(.text)
        *(.rodata)
        *(.data)
        *(.bss)

        __kernel_end = .;  /* Marca o fim do kernel */
    }

    .kernel_heap (NOLOAD) :
    {
        __heap_start = .;
        . += 16M;           /* Reserva 16 MiB para heap do kernel */
        __heap_end = .;
    }

    /* Espaço para usuário: o resto do espaço não é definido aqui no linker */
}
