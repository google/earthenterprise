#! /bin/bash

# This script launches the Fusion UI client in a running Open GEE Docker
# container.

# Set these variables before running this script to override the
# default values below:

# Set this to the OS distribution you want to run inside the Docker container.
# (Choose from the couple of available ones.)  You don't need to set this, if
# you are going to override the IMAGE_TAG variable.
: ${OS_DISTRIBUTION:="ubuntu-14"}

: ${STAGE_1_NAME:="clean-clone"}

if [ "$STAGE_1_NAME" == "clean-clone" ]; then
    CLONE_SUFFIX=""
else
    CLONE_SUFFIX="-$STAGE_1_NAME"
fi

# Set this to the name of the Open GEE Docker image to seach for containers
# running that image:
: ${IMAGE_TAG:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}"}

# Set this to the ID of a running container to skip searching by image tag,
# or, if you have more than one container running the Open GEE Docker image:
: ${CONTAINER_ID:=""}


SELF_NAME=$(basename "$0")

function echo_container_id()
{
    # `docker ps --filter "ancestor=. . ."` is currently broken, so we grep.
    docker ps --no-trunc=true | grep -E "^[a-zA-Z0-9]+\\s+$(printf '%q' "$IMAGE_TAG")[\\s:]" |
    while read -r id_field _; do
        if [ -n "$CONTAINER_ID" ]; then
            echo "$SELF_NAME: Multiple instances of the GEE image found. " >&2
            echo "Please, set the CONTAINER_ID variable to specify a container." >&2
            exit 1
        fi
        CONTAINER_ID="$id_field"
        echo "$id_field"
    done
}

if [ -z "$CONTAINER_ID" ]; then
    # Find the ID of the container derived from the GEE Docker image.
    CONTAINER_ID=$(echo_container_id)
    [ "$?" != "0" ] && exit 1
    if [ -z "$CONTAINER_ID" ]; then
        echo "$SELF_NAME: Couldn't find container for image with tag $IMAGE_TAG." >&2
        exit 1
    fi
fi

echo "Executing in container $CONTAINER_ID."

docker exec -ti "$CONTAINER_ID" "/root/opengee/bin/start-fusion.sh" 
