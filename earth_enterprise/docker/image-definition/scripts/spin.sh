#! /bin/bash

# This script never terminates.  However, it exits cleanly upon receiving a
# SIGTERM.  Therefore, if it is run as PID 1, it can be used to create a
# Docker container that waits until it's gracefully stopped by `docker stop`.

trap 'exit 0' SIGTERM

while true; do
    sleep 10
done
