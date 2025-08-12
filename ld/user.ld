SECTIONS
{
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
  .text 0x80002800 : {
    __ime_user_start = .;
    *(.text.__bootstrap)
    *(.text)
    *(.text.*)
    . = ALIGN(2048);
  } > iram
}