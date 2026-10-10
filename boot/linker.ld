OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

PHDRS
{
    text PT_LOAD FLAGS(5); /* read + execute */
    data PT_LOAD FLAGS(6); /* read + write */
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text :
    {
        KEEP(*(.text.boot))
        *(.text .text.*)
    } > RAM :text

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    } > RAM :text

    .data : ALIGN(4K)
    {
        *(.data .data.*)
    } > RAM :data

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    } > RAM :data

    .stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = .;
        . += 16K;
        __stack_top = .;
    } > RAM :data

    . = ALIGN(16);
    __heap_start = .;
    __heap_end = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
    }
}
