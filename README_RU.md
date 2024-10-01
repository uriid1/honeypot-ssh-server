# Honeypot SSH-Server
Russian | [English](README.md)</br>
Программа поставляется "как есть" и без каких-либо гарантий. </br>
За основу взята библиотека `LibSSH` и пример под названием `ssh_server_fork`.

Пример лога `log/honeypot_ssh.log`</br>
```
[2024-10-01 22:35:05] New session | IP: 85.21.81.22
[2024-10-01 22:35:10] Client enter password | IP: 85.21.81.22 | User: root | Pass: Admin123
[2024-10-01 22:35:12] Client enter password | IP: 85.21.81.22 | User: root | Pass: root
[2024-10-01 22:35:15] Client enter password | IP: 85.21.81.22 | User: admin | Pass: adminadmin
```

# Установка
### LibSHH
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
2. В домашнем каталоге появится директория `honeypot-ssh-server`, где будет бинарный файл сервера.</br>
3. Порт по умолчанию: `22`. Его необходимо будет открыть через ваш фаервол.</br>
4. Создайте сервис в `systemd` или запустите с root правами сервер `sudo ./honeypot-ssh-server`
5. Чтобы автоматически создать сервис, выполните `bash make-systemd-service.sh`

# Ручная сборка
Перед сборкой, можно изменить порт или путь к логам, для этого нужно внести изменения в файл `src/config.h`.</br>
На текущий момент не сделана поддержка аргументов, поэтому вся настройка вынесена в конфиг.</br>

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
