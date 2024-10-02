# Honeypot SSH-Server
[Russian](README_RU.md) | English</br>
The program is provided "as is" and without any warranties. </br>
It is based on the LibSSH library and the example called ssh_server_fork.
`By default, a database is created where all logs are written to /var/lib/honeypot-ssh/ssh_trap.db`

Example log at `/var/log/honeypot-ssh/honeypot.log`</br>
```
[2024-10-02 02:43:48] New session | IP: 43.155.130.118
[2024-10-02 02:43:48] New session | IP: 34.101.245.3
[2024-10-02 02:43:49] New session | IP: 45.183.218.125
[2024-10-02 02:43:49] Client enter password | IP: 43.155.130.118 | User: root | Pass: babu
[2024-10-02 02:43:50] Client enter password | IP: 34.101.245.3 | User: root | Pass: hello1
[2024-10-02 02:43:50] Client enter password | IP: 45.183.218.125 | User: root | Pass: Pizza@123
[2024-10-02 02:43:50] Client message | IP: 45.183.218.125 | Received SSH_MSG_DISCONNECT: 11:Bye Bye
```

# Using the Release (Binary)
Create all necessary directories and the systemd service
```bash
sudo mkdir -p /var/lib/honeypot-ssh/
sudo mkdir -p /var/log/honeypot-ssh/
sudo mkdir -p /etc/honeypot-ssh/
sudo ssh-keygen -t ecdsa -b 521 -f /etc/honeypot-ssh/ssh_host_ecdsa_key -N ""

tar -xzvf honeypot-ssh-server-linux-amd64.tar.gz
sudo cp honeypot-ssh-server /usr/local/bin/

bash make-systemd-service.sh
```

# Installation
### Dependencies
To compile, you need the packages: `libssh`, `openssl`, `sqlite3` .</br>
You should also have the compiler `gcc` and `make`.

### Debian 12
For new versions of Debian, the package name may change to `libssh-5`
```bash
sudo apt install libssh-4 libssh-dev
sudo apt install openssl
sudo apt install libsqlite3-dev
```

### Arch / Manjaro
```bash
sudo pacman -S libssh
sudo pacman -S openssl
sudo pacman -S sqlite3
```

# Compilation
### Build the binary and install
```bash
make
make install
```

### Create systemd and start the service
```bash
make install-service
```

# Uninstallation
```bash
make uninstall
make uninstall-service
```
