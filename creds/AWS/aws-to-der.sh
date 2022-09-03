#!/usr/bin/bash

# Convert AWS CA Certificates to DER format
openssl x509 -in ./AmazonRootCA1.pem -inform PEM -outform DER -out ./AmazonRootCA1.der
openssl x509 -in ./AmazonRootCA3.pem -inform PEM -outform DER -out ./AmazonRootCA3.der

# Expose certificates infos
openssl x509 -in ./AmazonRootCA1.der -inform DER -text -noout -out ./AmazonRootCA1.txt
openssl x509 -in ./AmazonRootCA3.der -inform DER -text -noout -out ./AmazonRootCA3.txt

# Convert AWS Certificates to PEM format
dir=caniot-controller
cert="9e99599b7b81772d43869b44f1ff4495be6e5c448b020c0a29a87e133c145c0d-certificate.pem.crt"
key="9e99599b7b81772d43869b44f1ff4495be6e5c448b020c0a29a87e133c145c0d-private.pem.key"

cp $dir/$cert $dir/cert.pem
cp $dir/$key $dir/key.pem

openssl x509 -in ./$dir/$cert -inform PEM -outform DER -out ./$dir/cert.der
openssl x509 -in ./$dir/$cert -inform PEM -text -noout -out ./$dir/cert.txt
openssl rsa -in ./$dir/$key -inform PEM -outform DER -out ./$dir/key.der
openssl rsa -in ./$dir/$key -inform PEM -text -noout -out ./$dir/key.txt