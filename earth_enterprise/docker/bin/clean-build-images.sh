#! /bin/bash

# This scripts deletes temporary Docker images used to build the final Open
# GEE Docker image.

SELF_DIR=$(dirname "$0")

source "$SELF_DIR/lib/input-variables.sh"


# Look at `build_image_parse_input_variables` in <lib/input-variables.sh> for
# input variables you can set before running this script.

build_image_parse_input_variables

docker rmi -f "$STAGE_2_IMAGE_NAME"
docker rmi -f "$STAGE_1_IMAGE_NAME"
