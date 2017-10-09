#! /bin/bash

# This script launches the Fusion UI client in a running Open GEE Docker
# container.

SELF_DIR=$(dirname "$0")

source "$SELF_DIR/lib/input-variables.sh"


# Look at `build_image_parse_input_variables` in <lib/input-variables.sh> for
# input variables you can set before running this script.

build_image_parse_input_variables

# Set this to the ID of a running container to skip searching by image tag,
# or, if you have more than one container running the Open GEE Docker image:
: ${CONTAINER_ID:=""}

# Set `GEE_IMAGE_NAME` to the name of the Open GEE Docker image to seach for
# containers running that image.  You don't have to set this, if you set
# `CONTAINER_ID`.
if [ -z "$GEE_IMAGE_NAME" ]; then
    if [ -n "$(docker images -q "opengee-experimental")" ]; then
        GEE_IMAGE_NAME="opengee-experimental"
    else
        GEE_IMAGE_NAME="thermopylae/opengee-experimental"
    fi
fi



SELF_NAME=$(basename "$0")

function echo_container_id()
{
    # `docker ps --filter "ancestor=. . ."` is currently broken, so we grep.
    docker ps --no-trunc=true | grep -E "^[a-zA-Z0-9]+\\s+$(printf '%q' "$GEE_IMAGE_NAME")[[:space:]:]" |
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
        echo "$SELF_NAME: Couldn't find container for image with tag $GEE_IMAGE_NAME." >&2
        exit 1
    fi
fi

echo "Executing in container $CONTAINER_ID."

docker exec -ti "$CONTAINER_ID" "/root/opengee/bin/start-fusion.sh" 
