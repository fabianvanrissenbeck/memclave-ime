# Benchmarks of Cryptographic Primitives on a DPU

All measurement where made with the compile flag `-Os` and
compiled using UPMEM's most recent (2025.1) SDK.

**Size of Cryptographic Primitives in IRAM**

|                  Primitive                   | Size in IRAM | Percent of IRAM |
|:--------------------------------------------:|-------------:|----------------:|
|               SHA256 (mbedtls)               |   6684 Bytes |           27.20 |
| DHKE (Naive Implementation - 2048 bit group) |   4464 Bytes |           18.16 |
|          Chacha20Poly1305 (mbedtls)          |  10968 Bytes |           33.47 |

**Performance of Chacha20-Poly1305**

| Implementation | Instructions |   Cycles | Estimated Speed per Thread |
|:--------------:|-------------:|---------:|---------------------------:|
|    mbedtls     |      1584880 | 18144736 |                  154 KiB/s |
