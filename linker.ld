ENTRY(_start)

SECTIONS
{
    /* Start putting things at physical 1 Megabyte */
    . = 1M;

    .boot_data : ALIGN(4K)
    {
        *(.boot_data)
    }

    .boot_text : ALIGN(4K)
    {
        *(.boot_text)
    }

    /* Shift the location counter up to 3 Gigabytes */
    . += 0xC0000000;

    /* AT(...) tells GRUB to keep loading it continuously in physical RAM */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)

        /* Safely allocate our page structures directly inside the BSS */
        . = ALIGN(4096);
        _kernel_page_dir = .;
        . += 4096;
        _kernel_page_table = .;
        . += 4096;
    }

    _heap_start = ALIGN(4K);
    _kernel_end = .;
}