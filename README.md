# Honeypot SSH-Server
[Russian](README_RU.md) | English</br>
The program is provided "as is" and without any warranties. </br>
It is based on the LibSSH library and the example called ssh_server_fork.

Log example `log/honeypot_ssh.log`</br>
```
[2024-10-01 22:35:05] New session | IP: 85.21.81.22
[2024-10-01 22:35:10] Client enter password | IP: 85.21.81.22 | User: root | Pass: Admin123
[2024-10-01 22:35:12] Client enter password | IP: 85.21.81.22 | User: root | Pass: root
[2024-10-01 22:35:15] Client enter password | IP: 85.21.81.22 | User: admin | Pass: adminadmin
```

# Installation
### LibSSH
First, install libssh on your distribution.</br>
You should also have the gcc compiler installed.

### Debian
For newer versions of Debian, the package name may change to `libssh-5`.
```bash
sudo apt install libssh-4 libssh-dev
```

### Arch / Manjaro
```bash
sudo pacman -S libssh
```

# Automatic Build
1. Run
```bash
bash install.sh
```
1. A `honeypot-ssh-server` directory will appear in your home folder containing the server binary.</br>
2. Default port: `22`. You will need to open it through your firewall.</br>
3. Create a systemd service or run the server with root privileges: `sudo ./honeypot-ssh-server`.
5. To create a service by default, run `bash make-systemd-service.sh`

# Manual Build
Before building, you can change the port or log path by modifying the `src/config.h` file.</br>
Currently, argument support is not implemented, so all configuration is done through the config file.</br>

### Generate Keys
```bash
ssh-keygen -t ecdsa -b 521 -f keys/ssh_host_ecdsa_key -N ""
```

### Finally, run
```bash
make
```

### File Copying
The executable file `honeypot-ssh-server` should be built.</br>
It is assumed that the server will be placed in your home directory with the following structure:</br>
```
honeypot-ssh-server
  - log
  - keys
  - honeypot-ssh-server
```

To do this, copy all the necessary files
```bash
mkdir ~/honeypot-ssh-server &&
  cp -r log ~/honeypot-ssh-server &&
  cp -r keys ~/honeypot-ssh-server &&
  cp honeypot-ssh-server ~/honeypot-ssh-server
```
