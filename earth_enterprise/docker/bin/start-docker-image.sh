#! /bin/bash

# Set these variables before running this script to override the
# default values below:
: ${GEE_SERVER_PORT:=80}
: ${HOST_GEVOL_PATH:="/host-gevol"}
: ${IMAGE_TAG:="opengee-experimental"}
: ${TEMPORARY_SERVERS:=""}

if [ -n "$TEMPORARY_SERVERS" ]; then
    MAP_GEVOL_PARAMETERS=""
else
    MAP_GEVOL_PARAMETERS="-v /gevol:$(printf '%q' "$HOST_GEVOL_PATH")"
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
    -it "${IMAGE_TAG}" \
    $@