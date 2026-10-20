/* SPDX-License-Identifier: MPL-2.0 */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
    . = 0x80000000;
    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    .data : ALIGN(4096) { *(.data .data.*) *(.sdata .sdata.*) }
    .bss (NOLOAD) : ALIGN(4096) {
        __bss_start = .;
        *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON)
        . = ALIGN(16); __bss_end = .;
    }
    .stacks (NOLOAD) : ALIGN(16) {
        . += 16384; __mstack_top = .;
        . += 32768; __sstack_top = .;
    }
    ASSERT(. < 0x88000000, "microkernel exceeds QEMU RAM")
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
