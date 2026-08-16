/* avr.ld — linker script for CSSC v6 native on AVR atmega328p.
 *
 * Maps:
 *   text   = 32 KiB flash @ 0x00000000 — code + read-only data
 *   data   = 2 KiB SRAM (less 256 B reserved for the stack) @ 0x00800100
 *
 * The data origin uses the LLVM/AVR convention of biasing data-space
 * addresses by 0x00800000 so the linker can distinguish flash from
 * SRAM symbols in the same script.
 *
 * Sections emitted:
 *   .vectors  → flash 0x0000+, must hold the 26-entry interrupt-vector
 *               table for atmega328p (jmp instructions, 4 bytes each).
 *   .text     → flash after vectors
 *   .rodata   → flash after text (immutable constants live here)
 *   .data     → SRAM 0x100+ (initialised data; loaded from flash via
 *               the boot stub)
 *   .bss      → SRAM after .data, zero-init by the boot stub
 *
 * For Phase 5d's first-cut the vector table is minimal: vector 0
 * (RESET) jmps to cssc_avr_entry, all 25 others jmp to
 * __bad_interrupt (an infinite loop). That covers the simulator
 * smoke-test and any sketch that doesn't enable interrupts.
 */

OUTPUT_FORMAT("elf32-avr")
OUTPUT_ARCH(avr:5)
ENTRY(cssc_avr_entry)

MEMORY {
  text (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
  data (rw!x): ORIGIN = 0x00800100, LENGTH = 2K - 256
}

SECTIONS {
  /* Interrupt-vector table */
  .vectors : {
    KEEP(*(.vectors))
    KEEP(*(.vectors.*))
  } > text

  /* Code */
  .text : {
    *(.progmem.gcc*)
    *(.progmem*)
    *(.text*)
  } > text

  /* Constants in flash */
  .rodata : {
    *(.rodata*)
  } > text

  /* Initialised globals — copied from flash into SRAM by the boot stub */
  .data : AT(ADDR(.rodata) + SIZEOF(.rodata)) {
    PROVIDE(__data_start = .);
    *(.data*)
    PROVIDE(__data_end = .);
  } > data

  /* Zero-init data */
  .bss : {
    PROVIDE(__bss_start = .);
    *(.bss*)
    *(COMMON)
    PROVIDE(__bss_end = .);
  } > data

  PROVIDE(__data_load_start = LOADADDR(.data));
  PROVIDE(__data_load_end = LOADADDR(.data) + SIZEOF(.data));

  /DISCARD/ : {
    *(.note*)
    *(.comment)
    *(.eh_frame*)
  }
}
