#! /bin/bash

# This scripts deletes temporary Docker images used to build the final Open
# GEE Docker image.

# Set these variables before running this script to override the
# default values below:
: ${OS_DISTRIBUTION:="ubuntu-14"}

: ${STAGE_1_NAME:="clean-clone"}

if [ "$STAGE_1_NAME" == "clean-clone" ]; then
    CLONE_SUFFIX=""
else
    CLONE_SUFFIX="-$STAGE_1_NAME"
fi

: ${STAGE_1_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}-stage-1"}
: ${STAGE_2_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}-stage-2"}

docker rmi -f "$STAGE_2_IMAGE_NAME"
docker rmi -f "$STAGE_1_IMAGE_NAME"
