PROVIDE(__ime_wram_start = 0x00000);
PROVIDE(__ime_wram_end = 0x10000);

SECTIONS
{
    .data 0x0 : {
        *(.data .data.*)
    }

    .data.stacks (NOLOAD) : {
        . = ALIGN(8);
        __sys_stack_thread_0 = .;
        . += STACK_SIZE_TASKLET_0;
    }

    .data.sw_cache (NOLOAD) : {
        . = ALIGN(8);
        __sw_cache_buffer = .;
        . += 8 * NR_TASKLETS;
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
    .text 0x80000000 : {
        *(.text.__bootstrap)
        *(.text)
        *(.text.*)
        __ime_core_end = .;
        . = ALIGN(2048);
        __ime_user_start = 0x80002800;
    } > iram
}