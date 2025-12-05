# The Memclave First-Stage-Loader (FSL)

The FSL kernel-image depends on the TL image and the key exchange
subkernels, as well as the messaging subkernel. These are linked
at compile time. To prepare them, compile the relevant subkernels
and TL and generate `.sk` images using the auth only option. The
FSL encrypts them with its own system key and needs them in plain
text. The TL image has to be further converted, as a raw IRAM image
is required instead of a `.sk` or elf file. Run
```
llvm-objcopy -O binary --only-section .text ./ime ./ime.iram.raw
```

Move all `.sk. images and the `ime.iram.raw` image into the `sk`
folder. Now the FSL can be compiled.

If you intend to debug a subkernel, you can replace the `msg.sk`
file with your own (auth-only) subkernel. Then run the fsl image
in UPMEM's usual debugger. You should probably place a debug
break `fault 0x3` at you main-functions entry point. Address
breakpoints are unreliable due to frequent reloading of code that
may alias your subkernels main address.