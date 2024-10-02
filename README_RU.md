# Honeypot SSH-Server
Russian | [English](README.md)</br>
Программа поставляется "как есть" и без каких-либо гарантий. </br>
За основу взята библиотека `LibSSH` и пример под названием `ssh_server_fork`.</br>
`По умолчанию создается база данных, куда пишется все логи /var/lib/honeypot-ssh/honeypot.db`

Пример лога `/var/log/honeypot-ssh/honeypot.log`</br>
```
[2024-10-02 02:43:48] New session | IP: 43.155.130.118
[2024-10-02 02:43:48] New session | IP: 34.101.245.3
[2024-10-02 02:43:49] New session | IP: 45.183.218.125
[2024-10-02 02:43:49] Client enter password | IP: 43.155.130.118 | User: root | Pass: babu
[2024-10-02 02:43:50] Client enter password | IP: 34.101.245.3 | User: root | Pass: hello1
[2024-10-02 02:43:50] Client enter password | IP: 45.183.218.125 | User: root | Pass: Pizza@123
[2024-10-02 02:43:50] Client message | IP: 45.183.218.125 | Received SSH_MSG_DISCONNECT: 11:Bye Bye
```

# Использование релиза (бинарника)
1. Загрузите последний релиз [honeypot-ssh-server-linux-amd64.tar.gz](https://github.com/uriid1/honeypot-ssh-server/releases)</br>
2. Создайте все необходимых директорий и сервис для systemd.
```bash
sudo mkdir -p /var/lib/honeypot-ssh/
sudo mkdir -p /var/log/honeypot-ssh/
sudo mkdir -p /etc/honeypot-ssh/
sudo ssh-keygen -t ecdsa -b 521 -f /etc/honeypot-ssh/ssh_host_ecdsa_key -N ""

tar -xzvf honeypot-ssh-server-linux-amd64.tar.gz
sudo cp honeypot-ssh-server /usr/local/bin/

bash make-systemd-service.sh
```

# Компиляция и установка
### Зависимости
Для компиляции нужны пакеты: `libssh`, `openssl`, `sqlite3` .</br>
У вас так же должны быть компилятор `gcc` и `make`.

### Debian 12
Для новых версий дебиана пакет может сменить название на `libssh-5`
```bash
sudo apt install libssh-4 libssh-dev
sudo apt install openssl
sudo apt install sqlite3 libsqlite3-dev
```

### Arch / Manjaro
```bash
sudo pacman -S libssh
sudo pacman -S openssl
sudo pacman -S sqlite3
```

# Компиляция
### Сборка бинарника и установка
```bash
make
make install
```

### Создание systemd и запуск сервиса
```bash
make install-service
```

# Работа с логами
Просмотр sqlite базы
```bash
sqlite3 /var/lib/honeypot-ssh/honeypot.db "SELECT * FROM logs ORDER BY id DESC LIMIT 50;" | column -t -s '|'
```

Включить запись в текстовый лог (`/var/log/honeypot-ssh/honeypot.log`)
```bash
honeypot-ssh-server --logging 1
```

Указать свой путь до лог файла
```bash
honeypot-ssh-server --logging 1 --path_log /home/$USER/honeypot.log
```

Отключить запись в базу данных (`/var/log/honeypot-ssh/honeypot.log`) и включить запись текстового лога
```bash
honeypot-ssh-server --sqlite 0 --logging 1 --path_log /home/$USER/honeypot.log
```

# Удаление
```bash
make uninstall
make uninstall-service
```
