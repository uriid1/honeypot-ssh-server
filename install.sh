#!/bin/env bash

printf "Generate Keys\n"
mkdir keys
ssh-keygen -t ecdsa -b 521 -f keys/ssh_host_ecdsa_key -N ""
printf "\n"

printf "Build\n"
make
printf "\n"

printf "Copy to /home/$USER/honeypot-ssh-server\n"
mkdir ~/honeypot-ssh-server
mkdir ~/honeypot-ssh-server/log
cp -r keys ~/honeypot-ssh-server
cp honeypot-ssh-server ~/honeypot-ssh-server
printf "\n"

printf "Done!\n"
printf "Now open the port (default is 22)\n"
printf "and run the server as root.\n"
