#! /bin/bash

# Set these variables before running this script to override the
# default values below:
: ${STAGE_1_IMAGE_NAME:="opengee-experimental-stage-1"}
: ${STAGE_2_IMAGE_NAME:="opengee-experimental-stage-2"}
: ${OUTPUT_IMAGE_NAME:="opengee-experimental"}
: ${STAGE_1_CONTAINER_NAME:="$STAGE_1_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}
: ${STAGE_2_CONTAINER_NAME:="$STAGE_2_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}

STAGE_COMPLETE_MESSAGE="Open GEE Docker Stage 2: Entering wait loop."

SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")

cd "$SELF_DIR/../image-definition" || exit 1
docker build -f Dockerfile.stage-1 --no-cache=true --rm=true -t "${STAGE_1_IMAGE_NAME}" .
docker run --cap-add=DAC_READ_SEARCH --name="$STAGE_1_CONTAINER_NAME" -ti "${STAGE_1_IMAGE_NAME}" |
while read ln; do
    echo "$ln"
    # Wait for install to complete:
    case "$(echo "$ln" | tr --delete '\r\n')" in
        "$STAGE_COMPLETE_MESSAGE")
            echo "$SELF_NAME: Flattening Docker image."
            docker export "$STAGE_1_CONTAINER_NAME" | docker import - "$STAGE_2_IMAGE_NAME"
            STAGE_2_CONTAINER_ID=$(docker run -d "$STAGE_2_IMAGE_NAME" /root/opengee/bin/spin.sh)
            docker commit --change="CMD /root/opengee/bin/start-servers.sh" "$STAGE_2_CONTAINER_ID" "$OUTPUT_IMAGE_NAME"
            ;;
    esac
done

echo "Open GEE Docker image built: $OUTPUT_IMAGE_NAME"
