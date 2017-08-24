#! /bin/bash

# This script builds a Docker image containing Open GEE Server and Fusion.

source "$(dirname "$0")/lib/input-variables.sh"
source "$(dirname "$0")/lib/image-building.sh"


# Look at `build_image_parse_input_variables` in <lib/input-variables.sh> for
# input variables you can set before running this script.

build_image_parse_input_variables

: ${OUTPUT_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}"}
: ${STAGE_1_CONTAINER_NAME:="$STAGE_1_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}
: ${STAGE_2_CONTAINER_NAME:="$STAGE_2_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}
: ${FLATTEN_IMAGE:="true"}
# Set TST_DOCKER_QUIET to a non-empty string to avoid prompting the user for input.

STAGE_COMPLETE_MESSAGE="Open GEE Docker Stage 2: Entering wait loop."

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


tst_docker_build_dockerfile_template \
    "$DOCKER_DIR/image-definition/Dockerfile.stage-1.$STAGE_1_NAME.template" \
    "${STAGE_1_IMAGE_NAME}"

docker run --cap-add=DAC_READ_SEARCH --name="$STAGE_1_CONTAINER_NAME" -ti "${STAGE_1_IMAGE_NAME}" |
while read ln; do
    echo "$ln"
    # Wait for install to complete:
    case "$(echo "$ln" | tr --delete '\r\n')" in
        "$STAGE_COMPLETE_MESSAGE")
            if is_string_false "$FLATTEN_IMAGE"; then
                echo "$SELF_NAME: Committing Docker image."
                docker commit "$STAGE_1_CONTAINER_NAME" "$OUTPUT_IMAGE_NAME"
                docker stop "$STAGE_1_CONTAINER_NAME"
            else
                echo "$SELF_NAME: Flattening Docker image."
                docker export "$STAGE_1_CONTAINER_NAME" | docker import - "$STAGE_2_IMAGE_NAME"
                STAGE_2_CONTAINER_ID=$(docker run -d "$STAGE_2_IMAGE_NAME" /root/opengee/bin/spin.sh)
                docker commit --change="CMD /root/opengee/bin/start-servers.sh" "$STAGE_2_CONTAINER_ID" "$OUTPUT_IMAGE_NAME"
                docker stop "$STAGE_2_CONTAINER_ID"
            fi
            ;;
    esac
done

echo "Open GEE Docker image built: $OUTPUT_IMAGE_NAME"
