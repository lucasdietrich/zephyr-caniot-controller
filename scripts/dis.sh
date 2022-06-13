#!/bin/bash

OUTPUT_DIR="./tmp"
ELF_PATH="build/zephyr/zephyr.elf"

mkdir -p $OUTPUT_DIR

arm-none-eabi-readelf -a $ELF_PATH > "$OUTPUT_DIR/readelf.txt"
arm-none-eabi-objdump -D $ELF_PATH > "$OUTPUT_DIR/objdump_all.S"
arm-none-eabi-objdump -d $ELF_PATH > "$OUTPUT_DIR/objdump.S"
arm-none-eabi-objdump -S $ELF_PATH > "$OUTPUT_DIR/objdump-source.S"
arm-none-eabi-objdump -S $ELF_PATH > "$OUTPUT_DIR/objdump-source.txt"
nm --print-size --size-sort --radix=x $ELF_PATH > "$OUTPUT_DIR/nm-source.txt"
arm-none-eabi-size -G -d $ELF_PATH > "$OUTPUT_DIR/size.txt"