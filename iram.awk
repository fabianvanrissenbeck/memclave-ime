/\.text[[:space:]]*PROGBITS/ {
    bytes_elf=strtonum("0x"$7)
    bytes_dpu=bytes_elf / 8 * 6
    inst=bytes_dpu / 6
    perc=inst / 4096 * 100

    printf "Bytes in MRAM:      %5d\n", bytes_elf
    printf "Bytes in IRAM:      %5d\n", bytes_dpu
    printf "Total Instructions: %5d\n", inst
    printf "Occupied:           %2.2f\n", perc
}