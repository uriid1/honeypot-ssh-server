default:
	gcc \
		src/honeypot-ssh-server.c \
		src/write_log.c \
		-o honeypot-ssh-server -lssh

static-release:
	mkdir build

	gcc \
		src/honeypot-ssh-server.c \
		src/write_log.c \
		-o build/honeypot-ssh-server \
		-Llib -lssh -lssl -lcrypto -lz -lgssapi_krb5

	cp -r lib/keys build/keys
	mkdir build/log
	tar -czvf honeypot-ssh-server-linux-amd64.tar.gz -C build .
	rm -rf build/

install:
	mkdir -p keys
	ssh-keygen -t ecdsa -b 521 -f keys/ssh_host_ecdsa_key -N ""
	mkdir -p ~/honeypot-ssh-server
	mkdir -p ~/honeypot-ssh-server/log
	cp -r keys ~/honeypot-ssh-server
	cp honeypot-ssh-server ~/honeypot-ssh-server
	bash make-systemd-service.sh

clean:
	rm -rf build
	rm honeypot-ssh-server
