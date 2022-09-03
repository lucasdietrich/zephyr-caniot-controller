#!/usr/bin/bash

days=3650

# use first argument
if [ -z "$1" ]; then
    echo "Usage: $0 <keysize>"
    exit 1
fi

rsa_key_size=$1
dir="rsa$rsa_key_size"

# check if ca key exists
if [ ! -f "./$dir/ca.key.pem" ]; then
    echo "CA key does not exist. Please generate it first."
    exit 1
fi

# check if csr exists
if [ ! -f "./$dir/csr.pem" ]; then
    echo "CSR does not exist. Please generate it first."
    exit 1
fi

# sign the csr
openssl x509 -req -in ./$dir/csr.pem -CA ./$dir/ca.cert.pem -CAkey ./$dir/ca.key.pem -CAcreateserial -out ./$dir/cert.pem -days $days

# convert pem cert to der
openssl x509 -in ./$dir/cert.pem -inform PEM -out ./$dir/cert.der -outform DER

# expose the cert
openssl x509 -in ./$dir/cert.pem -text -noout -out ./$dir/cert.txt