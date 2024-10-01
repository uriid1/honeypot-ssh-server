#!/bin/env bash

printf "Build\n"
make
make install
printf "\n"

printf "Done!\n"
printf "Now open the port (default is 22)\n"
printf "and run the server as root.\n"
