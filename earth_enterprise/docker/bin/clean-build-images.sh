#! /bin/bash

# This scripts deletes temporary Docker images used to build the final Open
# GEE Docker image.

# Set these variables before running this script to override the
# default values below:
: ${STAGE_1_IMAGE_NAME:="opengee-experimental-stage-1"}
: ${STAGE_2_IMAGE_NAME:="opengee-experimental-stage-2"}

docker rmi -f "$STAGE_2_IMAGE_NAME"
docker rmi -f "$STAGE_1_IMAGE_NAME"
