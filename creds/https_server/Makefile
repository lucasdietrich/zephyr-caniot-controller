keysize=4096
endpoint=192.0.2.1:443

all: rsa/cert.pem

rsa/ca.key.pem:
	./gen_rsa_ca.sh $(keysize)

rsa/csr.pem:
	./gen_rsa_csr.sh $(keysize)

rsa/cert.pem: rsa/ca.key.pem rsa/csr.pem
	./sign_rsa_csr.sh $(keysize)

ecdsa/ca.key.pem:
	./gen_ecdsa_ca.sh $(keysize)

new_cert:
	./gen_rsa_csr.sh $(keysize)
	./sign_rsa_csr.sh $(keysize)

connect:
	openssl s_client -connect $(endpoint) -cert rsa2048/cert.pem \
	-key rsa2048/key.pem -CAfile rsa2048/ca.cert.pem

connect_anonymous:
	openssl s_client -connect $(endpoint) -CAfile rsa2048/ca.cert.pem

clean_certs:
	rm -f rsa*/cert.der rsa*/cert.pem rsa*/cert.txt rsa*/csr.pem \
	rsa*/key.pem rsa*/key.der rsa*/key.txt 

clean:
	rm -f rsa*/*