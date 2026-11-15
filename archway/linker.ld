/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file linker.ld
 * @brief do i have to explain?
 */

OUTPUT_ARCH(loongarch)
ENTRY(_start)

PAGE_SIZE = 0x1000;

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0xffffffff80000000;
    __kernel_start = .;
    .text ALIGN(PAGE_SIZE) : ALIGN(PAGE_SIZE)
    {
        KEEP(*(.text.entry))
        *(.text .text.*)
        . = ALIGN(PAGE_SIZE);
    } :text
    .rodata ALIGN(PAGE_SIZE) : ALIGN(PAGE_SIZE)
    {
        *(.rodata .rodata.*)
        . = ALIGN(PAGE_SIZE);
    } :text
    .data ALIGN(PAGE_SIZE) : ALIGN(PAGE_SIZE)
    {
        *(.data .data.* .sdata .sdata.* .got .got.*)
        . = ALIGN(PAGE_SIZE);
    } :data
    .bss ALIGN(PAGE_SIZE) (NOLOAD) : ALIGN(PAGE_SIZE)
    {
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.*) *(COMMON)
        . = ALIGN(PAGE_SIZE);
        __bss_end = .;
    } :data
    __kernel_end = .;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
    .symtab 0 : { *(.symtab) }
    .shstrtab 0 : { *(.shstrtab) }
    .strtab 0 : { *(.strtab) }
}
