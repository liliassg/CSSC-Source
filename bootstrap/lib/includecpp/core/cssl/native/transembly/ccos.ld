/* CSSC v6 native — CCOS bare-metal kernel linker script (x86-64, Limine).
 * Higher-half kernel image. ENTRY(kmain) is the ELF e_entry that Limine
 * jumps to in long mode. Copied into the build dir by v6_driver._link_ccos
 * (same mechanism as avr.ld). Do not hand-edit a build-dir copy — edit this
 * source next to transembly/ccos_link parts. */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

PHDRS
{
    requests PT_LOAD;
    text     PT_LOAD;
    rodata   PT_LOAD;
    data     PT_LOAD;
}

SECTIONS
{
    /* Higher-half: top 2 GiB, matching -mcmodel=kernel. */
    . = 0xffffffff80000000;

    /* Limine request markers + request structs. KEEP so --gc-sections
     * never drops them; the bootloader scans this loaded segment. */
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.comment)
        *(.rela.*)
    }
}
