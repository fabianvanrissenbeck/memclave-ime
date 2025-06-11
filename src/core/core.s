.text
.globl __bootstrap

__bootstrap:
    jump zero, core_on_boot
    jump zero, core_on_replace

/* loads from address 0 in MRAM by default for testing purposes - assumes no tag verification */
core_on_boot:
    xor zero, id, 0x0, z, core_cont_boot
    stop true, __ime_user_start

core_cont_boot:
    move r0, 0x0
    move r1, 0x0
    move r2, 0x0

    jump zero, ime_replace_sk

/* used as a trampoline so that user code can always do jump 1 to call ime_replace_sk */
core_on_replace:
    jump zero, ime_replace_sk