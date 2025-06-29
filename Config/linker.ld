ENTRY(start)

SECTIONS
{
	. = 1M;

	.boot :
	{
		KEEP(*(.multiboot_header))
	}

	.text :
	{
		*(.text)
	} 

  .bump_heap (NOLOAD) :
    {
        __heap_start = .;
        . += 32M;               /* Reserva 32 MiB para heap */
        __heap_end = .;
    }
}
