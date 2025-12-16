MEMORY
{
    text (rx)  : ORIGIN = 0x0000, LENGTH = 16K
    data (rw!x): ORIGIN = 0x0100, LENGTH = 4K
}

SECTIONS
{
    .vectors :
    {
        KEEP(*(.vectors))
    } > text

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > text

    .data :
    {
        *(.data*)
        *(.bss*)
        *(COMMON)
    } > data
}
