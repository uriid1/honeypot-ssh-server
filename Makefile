# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lsqlite3 -lssh
SRCDIR = src
BUILDDIR = build
TARGET = honeypot-ssh-server

# Исходники
SRC = $(SRCDIR)/honeypot-ssh-server.c $(SRCDIR)/utils.c $(SRCDIR)/sql.c

# Статическая сборка (флаги для статического релиза)
# тут предполагается, что есть директория /lib
# в которой находятся libcrypto.a  libssh.a  libssl.a  sqlite3
# исходники или готовые бинарники библитек я не вижу смысла хранить в репозитории.
# Поэтому для сборки своего статического бинарника, нужно собрать libssh, openssl(libssl, libcrypto) и sqlite3.
STATIC_LDFLAGS = -Llib -lssh -lssl -lcrypto -lz -lgssapi_krb5 -lsqlite3

# Компиляция программы
default:
	$(CC) $(SRC) -o $(TARGET) $(LDFLAGS)

# Компиляция с отладкой
debug:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Цель для статической сборки
static-release: clean
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(SRC) -o $(BUILDDIR)/$(TARGET) $(STATIC_LDFLAGS)
	tar -czvf $(TARGET)-linux-amd64.tar.gz -C $(BUILDDIR) .
	rm -rf $(BUILDDIR)

# Установка
install:
	# База данных
	sudo mkdir -p /var/lib/honeypot-ssh/
	# Текстовые лог
	sudo mkdir -p /var/log/honeypot-ssh/
	# Ключ
	sudo mkdir -p /etc/honeypot-ssh/
	sudo ssh-keygen -t ecdsa -b 521 -f /etc/honeypot-ssh/ssh_host_ecdsa_key -N ""
	# Бинарник
	sudo cp honeypot-ssh-server /usr/local/bin/

# Удаление
uninstall:
	# Удаление бинарника
	sudo rm -f /usr/local/bin/honeypot-ssh-server
	# Удаление базы данных
	sudo rm -rf /var/lib/honeypot-ssh/
	# Удаление логов
	sudo rm -rf /var/log/honeypot-ssh/
	# Удаление ключей
	sudo rm -rf /etc/honeypot-ssh/

# Создание systemd сервиса
install-service:
	bash make-systemd-service.sh

# Удаление systemd сервиса
uninstall-service:
	sudo rm -f /etc/systemd/system/honeypot-ssh-server.service
	sudo systemctl disable honeypot-ssh-server.service
	sudo systemctl daemon-reload

# Очистка сборки
clean:
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)

.PHONY: default static-release install uninstall install-service uninstall-service service clean
