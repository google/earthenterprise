#! /bin/bash

# This script builds a Docker image containing Open GEE Server and Fusion.

source "$(dirname "$0")/lib/input-variables.sh"
source "$(dirname "$0")/lib/image-building.sh"


# Look at `build_image_parse_input_variables` in <lib/input-variables.sh> for
# input variables you can set before running this script.

build_image_parse_input_variables

: ${FLATTEN_IMAGE:="true"}
# Set TST_DOCKER_QUIET to a non-empty string to avoid prompting the user for input.

SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")
DOCKER_DIR="$SELF_DIR/.."

function is_string_false()
{
    case $(echo "$1" | tr '[:upper:]' '[:lower:]') in
        false|no|0)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

if is_string_false "$FLATTEN_IMAGE"; then
    FLATTEN_IMAGE=""
else
    FLATTEN_IMAGE="yes"
fi

tst_docker_build_dockerfile_template \
    "$DOCKER_DIR/image-definition/Dockerfile.stage-1.$STAGE_1_NAME.template" \
    "${GEE_IMAGE_NAME}" "$FLATTEN_IMAGE"

echo "Open GEE Docker image built: $GEE_IMAGE_NAME"
