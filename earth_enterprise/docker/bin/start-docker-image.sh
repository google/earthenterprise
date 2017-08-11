#! /bin/bash

# Set these variables before running this script to override the
# default values below:
: ${GEE_SERVER_PORT:=80}
: ${HOST_GEVOL_PATH:=""}
: ${OS_DISTRIBUTION:="ubuntu-14"}
: ${STAGE_1_NAME:="clean-clone"}

if [ "$STAGE_1_NAME" == "clean-clone" ]; then
    CLONE_SUFFIX=""
else
    CLONE_SUFFIX="-$STAGE_1_NAME"
fi

: ${IMAGE_TAG:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}"}

# Set the following to any extra flags you want to pass to `docker run`, such
# as sharing extra paths from the host system to the Docker container:
: ${DOCKER_RUN_FLAGS:=""}

if [ -z "$IMAGE_TAG" ]; then
    if [ -n "$(docker images -q "opengee-experimental")" ]; then
        IMAGE_TAG="opengee-experimental"
    else
        IMAGE_TAG="thermopylae/opengee-experimental"
    fi
fi

echo "IMAGE_TAG: $IMAGE_TAG"

if [ -z "$HOST_GEVOL_PATH" ]; then
    MAP_GEVOL_PARAMETERS=""
    TEMPORARY_SERVERS='true'
else
    MAP_GEVOL_PARAMETERS="-v $(printf '%q' "$HOST_GEVOL_PATH"):/gevol"
    TEMPORARY_SERVERS=''
fi

if [ -z "$DRI_PARAMETERS" -a -d '/dev/dri' ]; then
    for f in /dev/dri/*; do
        f_quoted=$(printf '%q' "$f")
        DRI_PARAMETERS="$DRI_PARAMETERS --device=$f_quoted:$f_quoted"
    done
else
    DRI_PARAMETERS=""
fi

echo "80:$GEE_SERVER_PORT"

docker run \
    -p "$GEE_SERVER_PORT:80" \
    $MAP_GEVOL_PARAMETERS \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    $DRI_PARAMETERS \
    --cap-add=DAC_READ_SEARCH \
    -e "TEMPORARY_SERVERS" \
    $DOCKER_RUN_FLAGS \
    -it "${IMAGE_TAG}" \
    $@