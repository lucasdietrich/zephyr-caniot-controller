# !/usr/bin/bash

# Get current directory
CURDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

PROJECTDIR=$CURDIR/../..

MCUBOOT_DIR=$PROJECTDIR/../bootloader/mcuboot
VENV_DIR=$PROJECTDIR/../.venv

PYTHON=$VENV_DIR/bin/python
IMGTOOL=$MCUBOOT_DIR/scripts/imgtool.py

source $VENV_DIR/bin/activate

# Generate private key
$IMGTOOL keygen -t ecdsa-p256 -k root-ec-p256.pem

# Generate public key C code (to be copied in boot/zephyr/keys.c)
$IMGTOOL getpub -k root-ec-p256.pem > root-ec-p256-pub.c