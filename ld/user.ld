SECTIONS
{
  .data.persist 0xffc0 - SIZEOF(.data.persist) (NOLOAD) : {
    __ime_persist_start = .;
    *(.data.persist)
    __ime_persist_end = .;
  }

  .data.threadio 0xffc0 (NOLOAD) : {
    __ime_threadio_start = .;
    . += 64;
    __ime_threadio_end = .;
  }

  .mram 0x08000000 : {
    *(.mram)
    *(.mram.*)
  } > mram

  .mram.noinit (NOLOAD) : {
    . = 0xbf00000;
    __ime_msg_sk = .;
    . += 128 << 10;
    __ime_xchg_sk = .;
    . += 128 << 10;
    __ime_scratch_a = .;
    . += 128 << 10;
    __ime_scratch_b = .;
    . += 128 << 10;
    __ime_msg_buf = .;
    . = 0xbffffc0;
    __ime_debug_out = .;
  } > mram

  /*
   * Merge every text section into .text.
   */
  .text 0x80002000 : {
    __ime_user_start = .;
    *(.text.__bootstrap)
    *(.text)
    *(.text.*)
    __ime_user_end = .;
    . = ALIGN(2048);
  } > iram
}