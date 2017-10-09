#! /bin/bash

# This script builds a Docker image containing a build environment for
# building Open GEE.

SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")

DOCKERFILE_TEMPLATES_DIR="$SELF_DIR/../image-definition"

source "$SELF_DIR/lib/image-building.sh"


# Set these variables before running this script to override the
# default values below:
: ${OS_DISTRIBUTION:="ubuntu-14"}
: ${IMAGE_NAME:="opengee-experimental-build-env-$OS_DISTRIBUTION"}

# Set TST_DOCKER_QUIET to a non-empty string to avoid prompting the user for input.


tst_docker_build_dockerfile_template \
    "$DOCKERFILE_TEMPLATES_DIR/Dockerfile.build-env.$OS_DISTRIBUTION.template" \
    "${IMAGE_NAME}" yes
