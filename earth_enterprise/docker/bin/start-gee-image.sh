#! /bin/bash

SELF_DIR=$(dirname "$0")

source "$SELF_DIR/lib/input-variables.sh"


# Set these variables before running this script to override the
# default values below.  Look at `build_image_parse_input_variables`
# in <lib/input-variables.sh> for additional input variables you can set before
# running this script.
: ${GEE_SERVER_PORT:=80}
: ${HOST_GEE_PERSISTENT_STORAGE_PATH:=""}
if [ -n "$HOST_GEE_PERSISTENT_STORAGE_PATH" ]; then
    : ${HOST_GEVOL_PATH:="$HOST_GEE_PERSISTENT_STORAGE_PATH/gevol"}
    : ${HOST_GEHTTPD_PATH:="$HOST_GEE_PERSISTENT_STORAGE_PATH/opt/google/gehttpd"}
    : ${HOST_GOOGLE_LOG_PATH:="$HOST_GEE_PERSISTENT_STORAGE_PATH/var/opt/google/log"}
    : ${HOST_PGSQL_DATA_PATH:="$HOST_GEE_PERSISTENT_STORAGE_PATH/var/opt/google/pgsql/data"}
else
    : ${HOST_GEVOL_PATH:=""}
    : ${HOST_GEHTTPD_PATH:=""}
    : ${HOST_GOOGLE_LOG_PATH:=""}
    : ${HOST_PGSQL_DATA_PATH:=""}
fi

build_image_parse_input_variables

echo "Persistent storage path: $HOST_GEE_PERSISTENT_STORAGE_PATH"

# Set the following to any extra flags you want to pass to `docker run`, such
# as sharing extra paths from the host system to the Docker container:
: ${DOCKER_RUN_FLAGS:=""}

TEMPORARY_SERVERS=""


# Get parameters for mapping persistent data paths:
if [ -z "$HOST_GEVOL_PATH" ]; then
    MAP_GEVOL_PARAMETERS=""
    TEMPORARY_SERVERS=yes
else
    printf -v MAP_GEVOL_PARAMETERS -- '-v %q:/gevol' "$HOST_GEVOL_PATH"
fi

if [ -z "$HOST_GEHTTPD_PATH" ]; then
    MAP_GEHTTPD_PARAMETERS=""
    TEMPORARY_SERVERS=yes
else
    printf -v MAP_GEHTTPD_PARAMETERS -- \
        "-v %q/htdocs/.htaccess:/opt/google/gehttpd/htdocs/.htaccess -v %q/logs:/opt/google/gehttpd/logs" \
        "$HOST_GEHTTPD_PATH" "$HOST_GEHTTPD_PATH"
fi

if [ -z "$HOST_GOOGLE_LOG_PATH" ]; then
    MAP_GOOGLE_LOG_PARAMETERS=""
    TEMPORARY_SERVERS=yes
else
    printf -v MAP_GOOGLE_LOG_PARAMETERS -- "-v %q:/var/opt/google/log" \
        "$HOST_GOOGLE_LOG_PATH"
fi

if [ -z "$HOST_PGSQL_DATA_PATH" ]; then
    MAP_PGSQL_DATA_PARAMETERS=""
    TEMPORARY_SERVERS=yes
else
    printf -v MAP_PGSQL_DATA_PARAMETERS -- \
        "-v %q:/var/opt/google/pgsql/data" "$HOST_PGSQL_DATA_PATH"
fi


# Get parameters for mapping DRI access:
if [ -z "$DRI_PARAMETERS" -a -d '/dev/dri' ]; then
    for f in /dev/dri/*; do
        f_quoted=$(printf '%q' "$f")
        DRI_PARAMETERS="$DRI_PARAMETERS --device=$f_quoted:$f_quoted"
    done
else
    DRI_PARAMETERS=""
fi


echo "Running Docker image: $GEE_IMAGE_NAME"
echo "Mapping Docker container port 80 to host port ${GEE_SERVER_PORT}."

docker run \
    -p "$GEE_SERVER_PORT:80" \
    $MAP_GEVOL_PARAMETERS \
    $MAP_GEHTTPD_PARAMETERS \
    $MAP_GOOGLE_LOG_PARAMETERS \
    $MAP_PGSQL_DATA_PARAMETERS \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    $DRI_PARAMETERS \
    --cap-add=DAC_READ_SEARCH \
    -e "TEMPORARY_SERVERS" \
    $DOCKER_RUN_FLAGS \
    -it "${GEE_IMAGE_NAME}" \
    $@