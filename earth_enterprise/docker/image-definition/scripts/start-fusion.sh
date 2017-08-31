#! /bin/bash

# Use this script to start Fusion in the Docker container.
SELF_DIR=$(dirname "$0")

"$SELF_DIR/create_x_user.sh"

: ${DISPLAY:=":0"}

su -c "DISPLAY=$(printf '%q' "$DISPLAY") /opt/google/bin/fusion" x_user