/* SPDX-License-Identifier: MPL-2.0 */
OUTPUT_ARCH(riscv)
ENTRY(user_main)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
    . = 0x400000;
    .text : { *(.text .text.*) } :text
    .rodata : ALIGN(16) { *(.rodata .rodata.*) } :text
    . = ALIGN(4096); __user_ro_end = .;
    .data : { *(.data .data.*) *(.sdata .sdata.*) } :data
    .bss (NOLOAD) : ALIGN(16) { *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON) } :data
    . = ALIGN(4096); __user_heap_start = .;
    ASSERT(. < 0x5ef000, "user image overlaps stack guard")
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
