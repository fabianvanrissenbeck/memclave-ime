# Script to extract IRAM image from elf file

#!/bin/bash

docker run --mount type=bind,src=./,dst=/project --rm fabianvanrissenbeck/upmem llvm-objcopy --only-section .text -O binary "/project/${1}" "/project/${2}"
