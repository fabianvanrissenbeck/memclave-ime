SECTIONS {
    .mram : {
        . = 0xbf00000;
        *g_sk_msg*(.mram)
        . = ALIGN(128 << 10);
        *g_sk_xchg_1*(.mram)
        . = ALIGN(128 << 10);
        *g_sk_xchg_2*(.mram)
        . = ALIGN(128 << 10);
        *g_sk_xchg_3*(.mram)
    } > mram

    .text : {
        *boot*(.text)
        *(.text)
        *(.text.*)
        // Make sure that the replace code cannot overwrite itself
        . = MAX(0x80002000, .);
        __text_replace_begin = .;
        *(.replace)
        __text_replace_end = .;
    } > iram
}