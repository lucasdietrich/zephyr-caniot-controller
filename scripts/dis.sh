#!/bin/bash

OUTPUT_DIR="./tmp"
ELF_PATH="build/zephyr/zephyr.elf"

mkdir -p $OUTPUT_DIR

arm-none-eabi-readelf -a $ELF_PATH > "$OUTPUT_DIR/readelf.txt"
arm-none-eabi-objdump -d $ELF_PATH > "$OUTPUT_DIR/objdump.S"
arm-none-eabi-objdump -S $ELF_PATH > "$OUTPUT_DIR/objdump-source.S"
