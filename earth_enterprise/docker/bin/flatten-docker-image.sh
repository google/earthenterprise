#! /bin/bash

# Set these variables before running this script to override the
# default values below:
: ${INPUT_IMAGE_TAG:="opengee-experimental-history"}
: ${OUTPUT_IMAGE_TAG:="opengee-experimental"}
: ${CONTAINER_NAME:="$INPUT_IMAGE_TAG-flatten-$(date '+%s.%N')-$RANDOM"}

SELF_DIR=$(dirname "$0")

docker run --name="$CONTAINER_NAME" -d "${INPUT_IMAGE_TAG}" /bin/bash && \
docker export "$CONTAINER_NAME" | docker import - "$OUTPUT_IMAGE_TAG"
