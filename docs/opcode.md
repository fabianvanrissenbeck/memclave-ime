# UPMEM's Opcodes for Instructions

UPMEM's instruction format is not documented, we try to reconstruct it
here. Understanding the format is necessary to ban certain target registers
and/or instructions.

We're going to ban the following list of instructions:
+ Thread control instructions `clr_run`, `boot`, `resume`.
+ Arithmetic instructions targeting `r20` or `r21`.
+ IRAM write access `ldmai`A.

## Thread Control Instructions

All banned thread control instructions share that their
highest 7 bits are equal, independent of their inputs. 
The ban test will check that the most significant byte
and `0x7C` won't be equal to `0x7C`.

```
boot r0, 0x7F, false, 0xFFFF  	01111101 10000011 00100000 11110111 11111111 11111111  	7d 83 20 f7 ff ff 
resume r23, 0x0, true, 0x0    	01111101 01011111 00100001 00000000 00000000 00000000  	7d 5f 21 00 00 00 
clr_run id, 0x12, smi, 0x1234 	01111100 11110011 00101111 00100001 00010010 00110100  	7c f3 2f 21 12 34 
```

## LDMAI

With the same parameters, all DMA operations have the exact same structure
except for the last two bits of the least significant byte of the instruction.
A possible check would be to check equality of the first 8 bits of the
most significant byte with `0x70` and the check that the two bits are
event numbers (so that the least significant bit is not set)

## Arithmetic

We need to ban instructions that target `r20`, `r21` and `d20`, the register
that encodes the former two as one target register. The target register
seems to be encoded in the first two most-significant bytes, when targeting
a normal register. When targeting a double register, the double register
is encoded as a 4-bit value in the most significant byte of the instruction.

It could be enough to check that MSB and `0x33` is not `0x22`.
