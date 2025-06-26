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
        . += 10M;               /* Reserva 1 MiB para heap */
        __heap_end = .;
    }
}
