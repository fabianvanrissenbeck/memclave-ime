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
}