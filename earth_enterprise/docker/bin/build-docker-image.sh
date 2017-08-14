#! /bin/bash

# This script builds a Docker image containing Open GEE Server and Fusion.

SELF_DIR=$(dirname "$0")

source "$SELF_DIR/lib/image-naming.sh"


# Set these variables before running this script to override the
# default values below:
: ${OS_DISTRIBUTION:="ubuntu-14"}

# Set this to a stage 1 template name (e.g. 'clean-clone' for the
# 'Dockerfile.stage-1.clean-clone.template').  The 'clean-clone' template
# clones from the Git Hub repository before building.  The 'current-clone'
# template copies the current files in your development repository.
: ${STAGE_1_NAME:="clean-clone"}

# Set this to the URL of the repository to clone during a "clean-clone"
# stage 1.  This variable is not used for "current-clone" stage 1.  If left
# blank, the default GEE Git URL will be used.
: ${CLEAN_CLONE_URL:=""}

# Set this to the name of a branch that you want to build during a
# "clean-clone" stage 1.  This variable is not used for the "current-clone"
# stage 1.
: ${CLEAN_CLONE_BRANCH:=""}

CLONE_SUFFIX=$(tst_docker_naming_echo_clone_suffix "$STAGE_1_NAME" "$CLEAN_CLONE_URL" "$CLEAN_CLONE_BRANCH")


: ${STAGE_1_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}-stage-1"}
: ${STAGE_2_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}-stage-2"}
: ${OUTPUT_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}"}
: ${STAGE_1_CONTAINER_NAME:="$STAGE_1_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}
: ${STAGE_2_CONTAINER_NAME:="$STAGE_2_IMAGE_NAME-build-$(date '+%s.%N')-$RANDOM"}
: ${FLATTEN_IMAGE:="true"}

STAGE_COMPLETE_MESSAGE="Open GEE Docker Stage 2: Entering wait loop."
DOCKERFILE_NO_CONTEXT_MARKER='# --\[ TST-Dockerfile: no-context \]--'

SELF_NAME=$(basename "$0")
SELF_DIR=$(dirname "$0")

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


cd "$SELF_DIR/../../.." || exit 1
DOCKER_DIR="earth_enterprise/docker"
DOCKERFILE_PATH="$DOCKER_DIR/Dockerfile.tmp.stage-1.$STAGE_1_NAME.${OS_DISTRIBUTION}${CLONE_SUFFIX}.$RANDOM"

export OS_DISTRIBUTION
export CLEAN_CLONE_URL_ESCAPED=$(printf "%q" "$CLEAN_CLONE_URL")
export CLEAN_CLONE_BRANCH_ESCAPED=$(printf "%q" "$CLEAN_CLONE_BRANCH")
envsubst < "$DOCKER_DIR/image-definition/Dockerfile.stage-1.$STAGE_1_NAME.template" \
    > "$DOCKERFILE_PATH"

if grep -q -x "$DOCKERFILE_NO_CONTEXT_MARKER" "$DOCKERFILE_PATH"; then
    DOCKER_CONTEXT_ARGUMENT="-"
else
    DOCKER_CONTEXT_ARGUMENT="."
fi
docker build --no-cache=true --rm=true \
    -t "${STAGE_1_IMAGE_NAME}" \
    $DOCKER_CONTEXT_ARGUMENT \
    <"$DOCKERFILE_PATH"

rm "$DOCKERFILE_PATH"

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
