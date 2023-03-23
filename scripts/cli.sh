#!/bin/bash

# Source Python3 virtual environment
source ../.venv/bin/activate

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Create an alias for the Python3 script
alias cli="python3 ${SCRIPT_DIR}/caniot-sdk/samples/cli.py -H 192.168.10.240 -p 80"