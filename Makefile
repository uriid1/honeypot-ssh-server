default:
	gcc \
		src/honeypot-ssh-server.c \
		src/write_log.c \
		-o honeypot-ssh-server -lssh
