#!/usr/bin/bash

days=3650

# use first argument
if [ -z "$1" ]; then
    echo "Usage: $0 <keysize>"
    exit 1
fi

# Check if key size is 1024, 2048 or 4096
if [ "$1" -eq 1024 ] || [ "$1" -eq 2048 ] || [ "$1" -eq 4096 ]; then
    echo "Generating key with size $1"
else
    echo "Key size must be 1024, 2048 or 4096"
    exit 1
fi

rsa_key_size=$1
dir="rsa$rsa_key_size"

# create a directory with key size for the key in the current script directory
mkdir -p $dir

# generate the key
openssl genrsa -out ./$dir/ca.key.pem $rsa_key_size

# convert pem key to der
openssl rsa -in ./$dir/ca.key.pem -outform DER -out ./$dir/ca.key.der

# expose the key
openssl rsa -in ./$dir/ca.key.pem -text -noout -out ./$dir/ca.key.txt

# generate the ca certificate from config
openssl req -new -x509 -key ./$dir/ca.key.pem -out ./$dir/ca.cert.pem -days $days -config ./ca.cnf

# convert pem ca to der
openssl x509 -in ./$dir/ca.cert.pem -inform PEM -out ./$dir/ca.cert.der -outform DER

# expose the ca
openssl x509 -in ./$dir/ca.cert.der -inform DER -text -noout -out ./$dir/ca.cert.txt