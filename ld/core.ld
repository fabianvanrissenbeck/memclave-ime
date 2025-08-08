PROVIDE(__ime_wram_start = 0x00000);
PROVIDE(__ime_wram_end = 0x10000);

SECTIONS
{
  /*
   * Merge every text section into .text.
   */
  .text 0x80000000 : {
    *(.text.__bootstrap)
    *(.text)
    *(.text.*)
    __ime_core_end = .;
    . = ALIGN(2048);
    __ime_user_start = 0x80002800;
  } > iram
}