#! /bin/bash

# This script sets up a regular user for starting X Windows applications
# in the Docker container, such as Fusion.

SELF_NAME=$(basename "$0")

if [ -z "$X_USER_ID" ]; then
    # Default to user ID 1000 for an X user.  Older versions of `useradd`
    # default to under-1000 UIDs (e.g. 500), which fail for most host systems.
    X_USER_ID=1000
fi

UID_ARGUMENT="--uid $X_USER_ID"


if [ -z "$X_USER_GID" ]; then
    # Default to group ID 1000 for an X user.  Older versions of `useradd`
    # default to under-1000 GIDs (e.g. 500), which fail for most host systems.
    X_USER_GID=1000
fi

GID_ARGUMENT="--gid $X_USER_GID"

# Add a group with the given group ID, if such doesn't exist:
if ! getent group "$X_USER_GID"; then
    groupadd --gid "$X_USER_GID" x_users
fi

if getent passwd "$X_USER_ID"; then
    echo "$SELF_NAME: X user with ID $X_USER_ID already exists. Quitting."
    exit 0
fi

useradd --create-home $UID_ARGUMENT $GID_ARGUMENT x_user

# Let the X user access the DRI device files for hardware video acceleration:
if [ -d /dev/dri ]; then
    for f in /dev/dri/*; do
        # Get the group ID of the file:
        GROUP_ID=$(stat -c %g "$f")
        # If a group with the given ID doesn't exist, create one:
        if [ ! $(getent group "$GROUP_ID") ]; then
            groupadd -g "$GROUP_ID" "video-$GROUP_ID"
        fi
        # Add the x_user to the group that owns the DRI device file:
        usermod --append --groups "$GROUP_ID" x_user
    done
fi
