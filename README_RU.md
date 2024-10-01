# Honeypot SSH-Server
Russian | [English](README.md)</br>
Программа поставляется "как есть" и без каких-либо гарантий. </br>
За основу взята библиотека `LibSSH` и пример под названием `ssh_server_fork`.

Пример лога `log/honeypot_ssh.log`</br>
```
[2024-10-02 02:43:48] New session | IP: 43.155.130.118
[2024-10-02 02:43:48] New session | IP: 34.101.245.3
[2024-10-02 02:43:49] New session | IP: 45.183.218.125
[2024-10-02 02:43:49] Client enter password | IP: 43.155.130.118 | User: root | Pass: babu
[2024-10-02 02:43:50] Client enter password | IP: 34.101.245.3 | User: root | Pass: hello1
[2024-10-02 02:43:50] Client enter password | IP: 45.183.218.125 | User: root | Pass: Pizza@123
[2024-10-02 02:43:50] Client message | IP: 45.183.218.125 | Received SSH_MSG_DISCONNECT: 11:Bye Bye
```

# Установка
### LibSSH
Для начала установите `libssh` в свой дистрибутив.</br>
У вас так же должны быть компилятор `gcc`.

### Debian
Для новых версий дебиана пакет может сменить название на `libssh-5`
```bash
sudo apt install libssh-4 libssh-dev
```

### Arch / Manjaro
```bash
sudo pacman -S libssh
```

# Автоматическая сборка
1. Выполните
```bash
bash install.sh
```
Или
```bash
make
make install
```
2. В домашнем каталоге появится директория `honeypot-ssh-server`, где будет бинарный файл сервера.</br>
3. Порт по умолчанию: `22`. Его необходимо будет открыть через ваш фаервол.</br>
4. Создайте сервис в `systemd` или запустите с root правами сервер `sudo ./honeypot-ssh-server`
5. Чтобы автоматически создать сервис, выполните `bash make-systemd-service.sh`

# Ручная сборка
Перед сборкой, можно изменить порт или путь к логам, для этого нужно внести изменения в файл `src/config.h`.</br>
Или `./honeypot-ssh-server --help` для справки по доступным аргументам.</br>

### Сгенерируйте ключи
```bash
ssh-keygen -t ecdsa -b 521 -f keys/ssh_host_ecdsa_key -N ""
```

### Наконец, выполните
```bash
make
```

### Копирование файлов
Должен собраться исполняемый файл `honeypot-ssh-server`.</br>
Предполагается, что сервер будет находится в домашней директории с такой иерархией:</br>
```
honeypot-ssh-server
  - log
  - keys
  - honeypot-ssh-server
```

Для этого скопируйте всё необходимое
```bash
mkdir ~/honeypot-ssh-server &&
  cp -r log ~/honeypot-ssh-server &&
  cp -r keys ~/honeypot-ssh-server &&
  cp honeypot-ssh-server ~/honeypot-ssh-server
```
