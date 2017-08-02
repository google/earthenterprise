#! /bin/bash

# This script sets up a regular user for starting X Windows applications
# in the Docker container, such as Fusion.

SELF_NAME=$(basename "$0")

if [ -n "$X_USER_ID" ]; then
    UID_ARGUMENT="--uid $X_USER_ID"
else
    UID_ARGUMENT=""
fi

if [ -n "$X_USER_GID" ]; then
    GID_ARGUMENT="--gid $X_USER_GID"
else
    GID_ARGUMENT=""
fi

if [ -n "$UID_ARGUMENT" ]; then
    if [ getent passwd "$X_USER_ID" ]; then
        echo "$SELF_NAME: X user with ID $X_USER_ID already exists. Quitting."
        exit 0
    fi
fi

useradd --create-home $UID_ARGUMENT $GID_ARGUMENT x_user

# Let the X user access the DRI device files for hardware video acceleration:
if [ -d /dev/dri ]; then
    for f in /dev/dri/*; do
        usermod --append --groups $(stat -c %g "$f") x_user
    done
fi
