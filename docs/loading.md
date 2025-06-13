# Secure Loading

On a high level, loading does the following:

1. Load a user defined subkernel address into the privileged register `r21`
   and the IRAM load address `__ime_user_start_i` into privileged register `r20`.
2. Check that no other threads are running.
3. Lock MRAM. (Fault)
4. Clear user memory by wiping WRAM.
5. Decrypt and verify the new subkernel in MRAM.
6. Scan the new subkernel in MRAM for bad instructions.
7. Load into WRAM.
8. Load into IRAM.
9. Unlock MRAM.
10. Set the privileged registers to a state, that causes DMA faults when directly jumpting to the `ldmai` address.

## Defenses

All steps between 2 and 9 can be built as a function call from
the main loader function. The only important
point here is that each of these subroutines performs no further
subroutine calls. This prevents anyone from changing the return
address mid-execution.

All these steps individually check that they are currently in system mode,
if necessary. This is especially the case for the IRAM loading and the
decryption component, as these impact security the most even if some
manipulation does eventually lead to a fault. These two should fault
as early as possible when a malicious action can be detected.

Checking whether the DPU is in system mode is  done by loading from MRAM
at address `r21`. This register only ever has a valid address when in system mode.
After each of these checks  one can make the assumption that all steps have
been executed correctly up to the current one.

## Files

+ `core.s` - Responsible for initialisation and bootup
+ `core_thrd.s` - Responsible for asserting no threads are alive.
+ `core_load.s` - Contains the core secure loader function
+ `core_mem.s` - WRAM wiping and MRAM locking and unlocking
+ `core_crypt.c` - In-MRAM decryption and authentication
+ `core_scan.c` - Scanning for banned instructions/registers
+ `core_wram.s` - Load primitive into WRAM
+ `core_iram.s` - Load primitive into IRAM
+ `core_util.s` - Utility to check whether in system mode and for standardized fault codes.
+ `core.h` - Header containing entrypoints and some descriptions.

## Global Registers

The loader uses two registers to store its parameters, additional
to the two privileged registers `r20` and `r21`. The registers chosen for that
are `r18` and `r19`. They may be used by users during subkernel execution.

| Register | Purpose | Restricted |
|:-----:|:----|:----:|
| `r17` | Set to anything but zero to wipe WRAM. | no |
| `r18` | Store address of 12 byte tag of a subkernel. | no |
| `r19` | Store address to the user key in WRAM. | no |
| `r20` | Store the IRAM loading address. Always 4096 * 8 outside the loader. | yes |
| `r21` | Store the subkernel location. Always 64 * 2^20 outside the loader. | yes |