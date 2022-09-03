keysize=4096

all: rsa/cert.pem

rsa/ca.key.pem:
	./gen_rsa_ca.sh $(keysize)

rsa/csr.pem:
	./gen_rsa_csr.sh $(keysize)

rsa/cert.pem: rsa/ca.key.pem rsa/csr.pem
	./sign_rsa_csr.sh $(keysize)

ecdsa/ca.key.pem:
	./gen_ecdsa_ca.sh $(keysize)

clean:
	rm -f rsa*/*