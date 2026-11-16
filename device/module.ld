/* Linker script for a Resona device module: one image linked at 0, code and read-only
 * data only. Link with -q so the pack builder sees the relocations. */
ENTRY(resona_module_get)

MEMORY
{
    IMAGE (rx) : ORIGIN = 0x0, LENGTH = 4M
}

SECTIONS
{
    .image : ALIGN(4)
    {
        KEEP(*(.resona_header))
        *(.text .text.*)
        *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4);
    } > IMAGE

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.riscv.attributes)
    }

    /* Anything that would need writable or initialised memory is a build error. */
    .forbidden :
    {
        *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON)
        *(.init_array* .ctors* .fini_array* .dtors*)
    } > IMAGE

    ASSERT(SIZEOF(.forbidden) == 0,
        "module has mutable globals or static constructors: keep state in the host-provided state block")
}
