#!/usr/bin/bash

# Convert AWS CA Certificates to DER format
openssl x509 -in ./AmazonRootCA1.pem -inform PEM -outform DER -out ./AmazonRootCA1.der
openssl x509 -in ./AmazonRootCA3.pem -inform PEM -outform DER -out ./AmazonRootCA3.der

# Expose certificates infos
openssl x509 -in ./AmazonRootCA1.der -inform DER -text -noout -out ./AmazonRootCA1.txt
openssl x509 -in ./AmazonRootCA3.der -inform DER -text -noout -out ./AmazonRootCA3.txt

# Convert AWS Certificates to PEM format
dir=qemu_caniot_controller
cid="4a03634d5ad6092781f4c7bef48620203f3ba75202835aebfdb61d15c8fb7f09"
cert="$cid-certificate.pem.crt"
key="$cid-private.pem.key"

echo $cert

cp $dir/$cert $dir/cert.pem
cp $dir/$key $dir/key.pem

openssl x509 -in ./$dir/$cert -inform PEM -outform DER -out ./$dir/cert.der
openssl x509 -in ./$dir/$cert -inform PEM -text -noout -out ./$dir/cert.txt
openssl rsa -in ./$dir/$key -inform PEM -outform DER -out ./$dir/key.der
openssl rsa -in ./$dir/$key -inform PEM -text -noout -out ./$dir/key.txt