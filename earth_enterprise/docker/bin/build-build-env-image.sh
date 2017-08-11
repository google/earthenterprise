#! /bin/bash

# This script builds a Docker image containing a build environment for
# building Open GEE.

# Set these variables before running this script to override the
# default values below:
: ${OS_DISTRIBUTION:="ubuntu-14"}
: ${IMAGE_NAME:="thermopylae/opengee-experimental-build-env-$OS_DISTRIBUTION"}


SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")

DOCKERFILE_NAME="Dockerfile.build-env.$OS_DISTRIBUTION"

cd "$SELF_DIR/../image-definition" || exit 1
docker build -f "$DOCKERFILE_NAME" --no-cache=true --rm=true -t "${IMAGE_NAME}" .
