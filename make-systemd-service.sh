#!/bin/env bash

readonly SERVICE_NAME="honeypot-ssh-server.service"

sudo -v
cat > /etc/systemd/system/$SERVICE_NAME <<EOF
[Unit]
Description=Honeypot SSH-server
Wants=network-online.target
After=network-online.target

[Service]
WorkingDirectory=/home/$USER/honeypot-ssh-server

# Run
ExecStart=/home/$USER/honeypot-ssh-server/honeypot-ssh-server
ExecReload=/home/$USER/honeypot-ssh-server/honeypot-ssh-server

# Restart
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl start $SERVICE_NAME
sudo systemctl enable $SERVICE_NAME
sudo -k
