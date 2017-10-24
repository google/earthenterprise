#! /bin/bash

# Use this script to start Fusion in the Docker container.
SELF_DIR=$(dirname "$0")

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --x-user-id=*)
            export X_USER_ID="${1#*=}"
            ;;
        --x-user-gid=*)
            export X_USER_GID="${#*=}"
            ;;
    esac
    shift
done

"$SELF_DIR/create_x_user.sh"

: ${DISPLAY:=":0"}

su -c "DISPLAY=$(printf '%q' "$DISPLAY") /opt/google/bin/fusion" x_user