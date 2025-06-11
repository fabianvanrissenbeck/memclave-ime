ENTRY(__bootstrap)
MEMORY {
    iram (rx) : ORIGIN = 0x80000000, LENGTH = 32K
    mram (wa) : ORIGIN = 0x08000000, LENGTH = 64M
    wram (wa) : ORIGIN = 0x00000000, LENGTH = 64K
}

PROVIDE(NR_TASKLETS = 16);
PROVIDE(STACK_SIZE_DEFAULT = 1024);


SECTIONS
{
  /*
   * Merge every text section into .text.
   */
  .text 0x80000800 : {
    __ime_user_start = .;
    *(.text.__bootstrap)
    *(.text)
    *(.text.*)
    . = ALIGN(2048);
  } > iram

  .data : {
    __ime_wram_start = .;
    *(*.data)
    *(*.data.*)
    . = ALIGN(8);
    __ime_stack_start = .;
    . += 1024;
    __ime_stack_end = .;
    . = ALIGN(2048);
    __ime_wram_end = ORIGIN(wram) + LENGTH(wram);
  } > wram

  .mram : {
    __ime_mram_start = .;
    . += 48 * 1024 * 1024;
    __ime_mram_end = .;
    __ime_swap_start = .;
    . += 16 * 1024 * 1024;
    __ime_wrap_end = .;
  } > mram

  /DISCARD/ : {
    *(.data.buffer)
  }
}