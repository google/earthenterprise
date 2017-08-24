# Source this file to get functions defined below.

SELF_PATH="${BASH_SOURCE}"
SELF_DIR=$(dirname "$SELF_PATH")

TST_DOCKER_REPOSITORY_DIR=$(cd "$SELF_DIR/../../../.." && pwd)
TST_DOCKERFILE_DIR="$TST_DOCKER_REPOSITORY_DIR/earth_enterprise/docker/image-definition"

source "$SELF_DIR/ui.sh"


TST_DOCKERFILE_NO_CONTEXT_MARKER='# --\[ TST-Dockerfile: no-context \]--'
TST_DOCKERFILE_CONTEXT_DIR_MARKER='^# --\[ TST-Dockerfile: context-dir=\K.*(?= \]--$)'
TST_DOCKERFILE_REQUIRED_VARS_MARKER='^# --\[ TST-Dockerfile: required-vars=\K.*(?= \]--$)'
TST_DOCKERFILE_REQUIRED_PASSWORD_VARS_MARKER='^# --\[ TST-Dockerfile: required-password-vars=\K.*(?= \]--$)'



function tst_docker_build_dockerfile_template()
{
    local DOCKERFILE_TEMPLATE_PATH="$1"
    local IMAGE_TAG="$2"
    
    local DOCKERFILE_PATH=$(mktemp "$TST_DOCKERFILE_DIR/Dockerfile.tmp.XXXXX")
    local DOCKER_CONTEXT_ARGUMENT
    local DOCKER_WORKING_DIR
    local REQUIRED_VARS
    local VAR

    if grep -q -x -e "$TST_DOCKERFILE_NO_CONTEXT_MARKER" "$DOCKERFILE_TEMPLATE_PATH"; then
        DOCKER_CONTEXT_ARGUMENT="-"
    else
        DOCKER_CONTEXT_ARGUMENT="-f $(printf "%q" "$DOCKERFILE_PATH") ."
    fi

    DOCKER_WORKING_DIR=$(grep -Po -e "$TST_DOCKERFILE_CONTEXT_DIR_MARKER" "$DOCKERFILE_TEMPLATE_PATH")
    if [ -n "$DOCKER_WORKING_DIR" -a "$DOCKER_CONTEXT_ARGUMENT" == "-" ]; then
        tst_docker_warn "Dockerfile species both no context, and context directory: $DOCKERFILE_TEMPLATE_PATH"
    fi
    if [ -z "$DOCKER_WORKING_DIR" ];then
        DOCKER_WORKING_DIR="$TST_DOCKER_REPOSITORY_DIR"
    elif [ "${DOCKER_WORKING_DIR#/}" == "$DOCKER_WORKING_DIR" ]; then
        # Context directories are relative to the repository directory:
        DOCKER_WORKING_DIR="$TST_DOCKER_REPOSITORY_DIR/$DOCKER_WORKING_DIR"
    fi

    REQUIRED_VARS=$(grep -Po -e "$TST_DOCKERFILE_REQUIRED_VARS_MARKER" "$DOCKERFILE_TEMPLATE_PATH")
    for VAR in $REQUIRED_VARS; do
        if [ -z "${!VAR}" ]; then
            if [ -n "$TST_DOCKER_QUIET" ]; then
                tst_docker_err "Required varible $VAR does not have a value while processing $DOCKERFILE_TEMPLATE_PATH"
                exit 1
            else
                tst_docker_read_variable "$VAR" "$VAR"
            fi
        fi
    done

    REQUIRED_VARS=$(grep -Po -e "$TST_DOCKERFILE_REQUIRED_PASSWORD_VARS_MARKER" "$DOCKERFILE_TEMPLATE_PATH")
    for VAR in $REQUIRED_VARS; do
        if [ -z "${!VAR}" ]; then
            if [ -n "$TST_DOCKER_QUIET" ]; then
                tst_docker_err "Required password varible $VAR does not have a value while processing $DOCKERFILE_TEMPLATE_PATH"
                exit 1
            else
                tst_docker_read_variable "$VAR" "$VAR" password
            fi
        fi
    done

    export OS_DISTRIBUTION
    export CLEAN_CLONE_URL_ESCAPED=$(printf "%q" "$CLEAN_CLONE_URL")
    export CLEAN_CLONE_BRANCH_ESCAPED=$(printf "%q" "$CLEAN_CLONE_BRANCH")
    export REDHAT_USERNAME_ESCAPED=$(printf "%q" "$REDHAT_USERNAME")
    export REDHAT_PASSWORD_ESCAPED=$(printf "%q" "$REDHAT_PASSWORD")
    envsubst < "$DOCKERFILE_TEMPLATE_PATH" > "$DOCKERFILE_PATH"

    (
        cd "$DOCKER_WORKING_DIR" || exit 1
        docker build --no-cache=true --rm=true \
            -t "${IMAGE_TAG}" \
            $DOCKER_CONTEXT_ARGUMENT \
            <"$DOCKERFILE_PATH"
    )

    rm "$DOCKERFILE_PATH"
}