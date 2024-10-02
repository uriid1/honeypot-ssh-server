#!/bin/env bash

readonly SERVICE_NAME="honeypot-ssh-server.service"

sudo touch /etc/systemd/system/$SERVICE_NAME
echo "[Unit]
Description=Honeypot SSH-server
Wants=network-online.target
After=network-online.target

[Service]
# Run
ExecStart=/usr/local/bin/honeypot-ssh-server
ExecReload=/usr/local/bin/honeypot-ssh-server

# Restart
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target" | sudo tee /etc/systemd/system/$SERVICE_NAME > /dev/null

sudo systemctl daemon-reload
sudo systemctl start $SERVICE_NAME
sudo systemctl enable $SERVICE_NAME
sudo -k
