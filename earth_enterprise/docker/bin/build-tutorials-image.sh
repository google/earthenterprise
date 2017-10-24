#! /bin/bash

# This script builds a Docker image containing Open GEE Server and Fusion.

source "$(dirname "$0")/lib/input-variables.sh"
source "$(dirname "$0")/lib/image-building.sh"


# Look at `build_image_parse_input_variables` in <lib/input-variables.sh> for
# input variables you can set before running this script.

build_image_parse_input_variables

: ${TUTORIALS_IMAGE_NAME:="${GEE_IMAGE_NAME}-tutorial-resources"}


SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")
DOCKER_DIR="$SELF_DIR/.."


export GEE_IMAGE_NAME

tst_docker_build_dockerfile_template \
    "$DOCKER_DIR/derived-images/Dockerfile.opengee-tutorial-resources.template" \
    "${TUTORIALS_IMAGE_NAME}" ""

echo "Open GEE tutorial resources Docker image built: $TUTORIALS_IMAGE_NAME"
