/* Linker script for a Resona box app: one image linked at 0 (code, read-only data,
 * initialised data and the static-constructor table) followed by .bss. Link with -q so
 * the pack builder sees the relocations. Unlike algorithm packs, apps may have mutable
 * globals and C++ static constructors: they are programs. */
ENTRY(resona_app_get)

MEMORY
{
    /* Room to link; the App size limit itself is the ASSERT below. */
    IMAGE (rwx) : ORIGIN = 0x0, LENGTH = 4096K
}

SECTIONS
{
    .image : ALIGN(4)
    {
        KEEP(*(.resona_header))
        *(.text .text.*)
        *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4);
        __resona_init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array .ctors))
        __resona_init_array_end = .;
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(16);
    } > IMAGE

    .bss (NOLOAD) : ALIGN(16)
    {
        *(.sbss .sbss.* .bss .bss.* COMMON)
        . = ALIGN(16);
    } > IMAGE

    /* An App (program and variables) is at most 512 KB on the Box; Lab checks it too. */
    ASSERT(SIZEOF(.image) + SIZEOF(.bss) <= 512K, "Resona App: the App is larger than 512 KB")

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.riscv.attributes)
        *(.fini_array* .dtors*)
    }
}
