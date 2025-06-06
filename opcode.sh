#!/bin/bash

if which dpu-clang > /dev/null 2> /dev/null;
then
  printf "$1" > /tmp/tmp.s
  dpu-clang -c -o /tmp/tmp.o /tmp/tmp.s
  llvm-objcopy -O binary --only-section .text /tmp/tmp.o /tmp/tmp.bin
  xxd -e -g 8 /tmp/tmp.bin | awk '{ print $2 }'
else
  docker run -it --mount type=bind,src=$0,dst=/script.sh,ro fabianvanrissenbeck/upmem:latest /script.sh "$1"
fi
