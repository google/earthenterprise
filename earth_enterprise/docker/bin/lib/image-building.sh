# Source this file to get functions defined below.

SELF_PATH="${BASH_SOURCE}"
SELF_DIR=$(dirname "$SELF_PATH")

TST_DOCKER_REPOSITORY_DIR=$(cd "$SELF_DIR/../../../.." && pwd)
TST_DOCKER_BUILD_DIR="$TST_DOCKER_REPOSITORY_DIR/earth_enterprise/docker/image-definition/build"
TST_DOCKER_LIB_DIR="$(cd "$SELF_DIR" && pwd)"

source "$TST_DOCKER_LIB_DIR/ui.sh"
source "$TST_DOCKER_LIB_DIR/shell-script-parsing.sh"


TST_DOCKERFILE_NO_CONTEXT_MARKER='# --\[ TST-Dockerfile: no-context \]--'
TST_DOCKERFILE_CONTEXT_DIR_MARKER='^# --\[ TST-Dockerfile: context-dir=\K.*(?= \]--$)'
TST_DOCKERFILE_REQUIRED_VARS_MARKER='^# --\[ TST-Dockerfile: required-vars=\K.*(?= \]--$)'
TST_DOCKERFILE_REQUIRED_PASSWORD_VARS_MARKER='^# --\[ TST-Dockerfile: required-password-vars=\K.*(?= \]--$)'


function tst_string_ends_with()
{
    local string="$1"
    local suffix="$2"

    [ "${string:$[ ${#string} - ${#suffix} ]}" = "$suffix" ]
}


function tst_docker_template_run_build_stage()
{
    local DOCKERFILE_PATH="$1"
    local DOCKER_WORKING_DIR="$2"
    local DOCKER_CONTEXT_ARGUMENT="$3"
    local IMAGE_TAG="$4"

    (
        # Run the Docker build:
        cd "$DOCKER_WORKING_DIR" || exit 1
        docker build --no-cache=true --rm=true \
            -t "${IMAGE_TAG}" \
            $DOCKER_CONTEXT_ARGUMENT \
            <"$DOCKERFILE_PATH"
    )
}

function tst_docker_inject_file_command()
{
    local DESTINATION_PATH="$1"
    local REPLY

    cat <<END
RUN mkdir -p $(printf "%q" "$(dirname "$DESTINATION_PATH")") && \\
    ( \\
END
    while read -r; do
        printf 'echo %q ; \\\n' "$REPLY"
    done
    cat <<END
) > $(printf "%q" "$DESTINATION_PATH")
END
}

# Rewrite the Dockerfile template, so that the Docker container has all the
# scripts we need.
function tst_docker_template_filter()
{
    local COMMAND_PATTERN='^[[:space:]]*\K[a-zA-Z-]+(?=(|[[:space:]].*)$)'
    local COMMAND_NAME
    local COMMAND_ARGUMENTS

    while read -r; do
        LINE="$REPLY"
        COMMAND_NAME=$(echo "$LINE" | grep -Po -e "$COMMAND_PATTERN")
        COMMAND_ARGUMENTS="${LINE#*$COMMAND_NAME}"
        COMMAND_NAME="${COMMAND_NAME^^}"

        if [ -z "$COMMAND_NAME" ]; then
            # Pass no-command lines:
            echo "$LINE"
            continue
        fi

        # Parse line continuations:
        if tst_string_ends_with "$LINE" "\\"; then
            read -r
            LINE="${REPLY}"
            while tst_string_ends_with "$LINE" "\\"; do
                printf -v COMMAND_ARGUMENTS '%s\n%s' "${COMMAND_ARGUMENTS}" "${LINE}"
                read -r
                LINE="${REPLY}"
            done
            printf -v COMMAND_ARGUMENTS '%s\n%s\n' "${COMMAND_ARGUMENTS}" "${LINE}"
        fi

        # Pass the command through:
        echo "${COMMAND_NAME}${COMMAND_ARGUMENTS}"

        case "$COMMAND_NAME" in
            FROM)
                echo
                echo "# Inject the environment variable client in the Docker image:"
                tst_docker_inject_file_command \
                    /opt/tst-docker/bin/lib/env-client.sh \
                    < "$TST_DOCKER_LIB_DIR/env-client.sh"
                echo
                ;;
        esac
    done
}

function tst_docker_run_arguments_insert_image_name()
{
    local TST_DOCKER_COMMAND_STRING="$1"
    local IMAGE_NAME="$2"
    local OUTPUT_VARIABLE_NAME="$3"

    local TST_DOCKER_ARGUMENT
    local HAS_IMAGE_NAME_BEEN_INSERTED=""
    local ARGUMENTS_STRING=""

    while [ -n "$TST_DOCKER_COMMAND_STRING" ]; do
        cmd_parse_first_arg "$TST_DOCKER_COMMAND_STRING" yes \
            TST_DOCKER_ARGUMENT TST_DOCKER_COMMAND_STRING || return 1
        case "$TST_DOCKER_ARGUMENT" in
            --[a-z]*)
                printf -v ARGUMENTS_STRING "%s %q" "$ARGUMENTS_STRING" "$TST_DOCKER_ARGUMENT"
                ;;
            -[a-z]*)
                printf -v ARGUMENTS_STRING "%s %q" \
                    "$ARGUMENTS_STRING" "$TST_DOCKER_ARGUMENT"
                cmd_parse_first_arg "$TST_DOCKER_COMMAND_STRING" yes \
                    TST_DOCKER_ARGUMENT TST_DOCKER_COMMAND_STRING || return 1
                printf -v ARGUMENTS_STRING "%s %q" \
                    "$ARGUMENTS_STRING" "$TST_DOCKER_ARGUMENT"
                ;;
            *)
                # We've reached the end of `docker run` options; time to 
                # insert the image name:
                printf -v ARGUMENTS_STRING "%s %q" \
                    "$ARGUMENTS_STRING" "$IMAGE_NAME"
                printf -v ARGUMENTS_STRING "%s %q" "$ARGUMENTS_STRING" \
                    "$TST_DOCKER_ARGUMENT"
                # And pass the command and arguments through:
                while [ -n "$TST_DOCKER_COMMAND_STRING" ]; do
                    cmd_parse_first_arg "$TST_DOCKER_COMMAND_STRING" yes \
                        TST_DOCKER_ARGUMENT TST_DOCKER_COMMAND_STRING \
                        || return 1
                    printf -v ARGUMENTS_STRING "%s %q" "$ARGUMENTS_STRING" \
                        "$TST_DOCKER_ARGUMENT"
                done

                HAS_IMAGE_NAME_BEEN_INSERTED=yes
                ;;
        esac
    done

    if [ -z "$HAS_IMAGE_NAME_BEEN_INSERTED" ]; then
        # There was no command to `run` specified.  Append image name:
        printf -v ARGUMENTS_STRING "%s %q" "$ARGUMENTS_STRING" "$IMAGE_NAME"
    fi

    # Set the output variable:
    printf -v "$OUTPUT_VARIABLE_NAME" "%s" "$ARGUMENTS_STRING"
}

function tst_docker_template_process_tst_instructions()
{
    local DOCKER_BUILD_DIR="$1"
    local DOCKER_WORKING_DIR="$2"
    local DOCKER_CONTEXT_ARGUMENT="$3"
    local IMAGE_TAG="$4"
    local FLATTEN_IMAGE="$5"

    local DOCKERFILE_PATH="$DOCKER_BUILD_DIR/Dockerfile"
    local COMMAND_PATTERN='^[[:space:]]*#[[:space:]]*TST-\K[a-zA-Z-]+(?=(|[[:space:]].*)$)'
    local COMMAND_NAME
    local COMMAND_ARGUMENTS
    local ARGUMENT
    local EXEC_ARGUMENTS
    local LAST_ARGUMENT
    local LINE
    local REPLY
    local STAGE_NUMBER=1
    local COUNTER
    local CONTAINER_ID
    local CONTAINER_CMD
    local CONTAINER_EXPOSE

    while read -r; do
        LINE="$REPLY"
        COMMAND_NAME=$(echo "$LINE" | grep -Po -e "$COMMAND_PATTERN")
        COMMAND_ARGUMENTS="${LINE#*$COMMAND_NAME}"
        COMMAND_NAME="${COMMAND_NAME^^}"

        if [ -z "$COMMAND_NAME" ]; then
            # Skip no-command lines:
            continue
        fi

        # Restore the "TST-" command name prefix:
        COMMAND_NAME="TST-${COMMAND_NAME}"

        # Trim leading whitespace:
        COMMAND_ARGUMENTS="${COMMAND_ARGUMENTS#"${COMMAND_ARGUMENTS%%[![:space:]]*}"}"

        # Parse line continuations:
        if tst_string_ends_with "$LINE" "\\"; then
            COMMAND_ARGUMENTS="${COMMAND_ARGUMENTS%\\}"
            read -r
            LINE="${REPLY#*#}"
            while tst_string_ends_with "$LINE" "\\"; do
                printf -v COMMAND_ARGUMENTS '%s %s' "${COMMAND_ARGUMENTS}" "${LINE%\\}"
                read -r
                LINE="${REPLY#*#}"
            done
            printf -v COMMAND_ARGUMENTS '%s %s' "${COMMAND_ARGUMENTS}" "${LINE}"
        fi

        case "$COMMAND_NAME" in
            TST-RUN-STAGE)
                # Construct an argument string for `docker run`:
                tst_docker_run_arguments_insert_image_name \
                    "$COMMAND_ARGUMENTS" \
                    "${IMAGE_TAG}-stage-${STAGE_NUMBER}" EXEC_ARGUMENTS
                echo "==================="
                echo "TST-RUN-STAGE docker run -t $EXEC_ARGUMENTS"
                echo "==================="
                # Obtain an image for the previous stage:
                if [ "$STAGE_NUMBER" -lt 2 ]; then
                    echo "Building initial stage: ${IMAGE_TAG}-stage-${STAGE_NUMBER}"
                    tst_docker_template_run_build_stage \
                        "$DOCKERFILE_PATH" "$DOCKER_WORKING_DIR" \
                        "$DOCKER_CONTEXT_ARGUMENT" \
                        "${IMAGE_TAG}-stage-${STAGE_NUMBER}" || return 1
                else
                    echo "Committing stage: ${IMAGE_TAG}-stage-${STAGE_NUMBER}"
                    docker commit $(cat "$DOCKER_BUILD_DIR/stage-${STAGE_NUMBER}.cid") \
                        "${IMAGE_TAG}-stage-${STAGE_NUMBER}" || return 1
                fi
                (( STAGE_NUMBER++ ))
                # Run the new stage:
                echo "Running new stage. Container ID file: ${DOCKER_BUILD_DIR}/stage-${STAGE_NUMBER}.cid"
                docker run --cidfile="${DOCKER_BUILD_DIR}/stage-${STAGE_NUMBER}.cid" \
                    -t $EXEC_ARGUMENTS || return 1
                ;;
            *)
                echo "Warning: Unrecognized TST Docker instruction ignored: $COMMAND_NAME." >&2
                ;;
        esac
    done

    if [ "$STAGE_NUMBER" -lt 2 ]; then
        # No stages in this Dockerfile:
        if [ -z "$FLATTEN_IMAGE" ]; then
            echo "Buliding image with no stages: ${IMAGE_TAG}"
            tst_docker_template_run_build_stage \
                "$DOCKERFILE_PATH" "$DOCKER_WORKING_DIR" \
                "$DOCKER_CONTEXT_ARGUMENT" \
                "${IMAGE_TAG}" || return 1
        else
            echo "Building image with no stages for flattening: ${IMAGE_TAG}-stage-$[ $STAGE_NUMBER + 1 ]"
            tst_docker_template_run_build_stage \
                "$DOCKERFILE_PATH" "$DOCKER_WORKING_DIR" \
                "$DOCKER_CONTEXT_ARGUMENT" \
                "${IMAGE_TAG}-stage-$[ $STAGE_NUMBER + 1 ]" || return 1
            (( STAGE_NUMBER++ ))
            echo "Creating a container from the built image. ID file: ${DOCKER_BUILD_DIR}/stage-${STAGE_NUMBER}.cid"
            docker run --cidfile="${DOCKER_BUILD_DIR}/stage-${STAGE_NUMBER}.cid" /bin/bash
        fi
    else
        if [ -z "$FLATTEN_IMAGE" ]; then
            echo "Committing final image: ${IMAGE_TAG}"
            docker commit $(cat "$DOCKER_BUILD_DIR/stage-${STAGE_NUMBER}.cid") \
                "${IMAGE_TAG}" || return 1
        fi
    fi
    if [ -n "$FLATTEN_IMAGE" ]; then
        echo "$SELF_NAME: Flattening Docker image."
        docker export "$(cat "$DOCKER_BUILD_DIR/stage-${STAGE_NUMBER}.cid")" |
            docker import - "${IMAGE_TAG}-stage-${STAGE_NUMBER}" || return 1
        # Restore the CMD and EXPOSE properties:
        CONTAINER_CMD=$(docker inspect --format="{{ json .Config.Cmd }}" "${IMAGE_TAG}-stage-1")
        CONTAINER_EXPOSE=$(docker inspect --format="{{ json .Config.ExposedPorts }}" "${IMAGE_TAG}-stage-1")
        CONTAINER_ID=$(docker run -d "${IMAGE_TAG}-stage-${STAGE_NUMBER}" /bin/bash)
        if [ "$CONTAINER_EXPOSE" != "null" ]; then
            docker commit --change="CMD $CONTAINER_CMD" \
                --change="EXPOSE $(echo "$CONTAINER_EXPOSE" | tr '{}:"' ' ')" "$CONTAINER_ID" \
                "${IMAGE_TAG}" || return 1
        else
            docker commit --change="CMD $CONTAINER_CMD" \
                "$CONTAINER_ID" "${IMAGE_TAG}" || return 1
        fi
        (( STAGE_NUMBER++ ))
    fi

    echo "Deleting stage images."
    (( COUNTER=1 ))
    while [ "$COUNTER" -lt "$STAGE_NUMBER" ]; do
        docker rmi -f "${IMAGE_TAG}-stage-${COUNTER}" || return 1
        (( COUNTER++ ))
    done
}


function tst_docker_build_dockerfile_template()
{
    local DOCKERFILE_TEMPLATE_PATH="$1"
    local IMAGE_TAG="$2"
    local FLATTEN_IMAGE="$3"

    mkdir -p "$TST_DOCKER_BUILD_DIR"

    local DOCKER_BUILD_DIR=$(mktemp -p "$TST_DOCKER_BUILD_DIR" -d tmp.XXXXX) || return 1
    local DOCKERFILE_PATH="$DOCKER_BUILD_DIR/Dockerfile"
    export SECRET_SERVER_DIR="$DOCKER_BUILD_DIR/secrets"
    export SECRET_SERVER_DIR_ESCAPED
    local SECRET_SERVER_SOCKET_PATH="$SECRET_SERVER_DIR/socket"
    local DOCKER_CONTEXT_ARGUMENT
    local DOCKER_WORKING_DIR
    local REQUIRED_VARS
    local SECRET_VARS
    local SUBSTITUTE_VARS
    local VAR

    printf -v SECRET_SERVER_DIR_ESCAPED "%q" "$SECRET_SERVER_DIR"
    mkdir -p "$SECRET_SERVER_DIR"

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
        printf -v "${VAR}_ESCAPED" "%q" "${!VAR}"
        export "$VAR" "${VAR}_ESCAPED"
    done

    SECRET_VARS=$(grep -Po -e "$TST_DOCKERFILE_REQUIRED_PASSWORD_VARS_MARKER" "$DOCKERFILE_TEMPLATE_PATH")
    for VAR in $SECRET_VARS; do
        if [ -z "${!VAR}" ]; then
            if [ -n "$TST_DOCKER_QUIET" ]; then
                tst_docker_err "Required password varible $VAR does not have a value while processing $DOCKERFILE_TEMPLATE_PATH"
                exit 1
            else
                tst_docker_read_variable "$VAR" "$VAR" password
            fi
        fi
        printf -v "${VAR}_ESCAPED" "%q" "${!VAR}"
        export "$VAR" "${VAR}_ESCAPED"
    done

    # Create a list of substituted variables that desn't include any secret variables:
    SUBSTITUTE_VARS=$(envsubst --variables "$(cat "$DOCKERFILE_TEMPLATE_PATH")")
    SECRET_VARS=$(for VAR in $SECRET_VARS; do echo "$VAR"; echo "${VAR}_ESCAPED"; done)
    # Subtract the two sets:
    SUBSTITUTE_VARS=$(comm -23 \
        <(echo "$SUBSTITUTE_VARS" | sort) <(echo "$SECRET_VARS" | sort))

    export OS_DISTRIBUTION
    tst_docker_template_filter < "$DOCKERFILE_TEMPLATE_PATH" |
        envsubst \
            "$(for VAR in $SUBSTITUTE_VARS; do [ -n "$VAR" ] && echo "\${$VAR}"; done)" > \
            "$DOCKERFILE_PATH"

    # Start serving secret variables in the background:
    ENVSERVE_PIPE_PATH="$SECRET_SERVER_SOCKET_PATH" \
        ENVSERVE_SOCKET_MODE=0600 \
        "$TST_DOCKER_LIB_DIR/env-server.sh" &

    tst_docker_template_process_tst_instructions \
        "$DOCKER_BUILD_DIR" "$DOCKER_WORKING_DIR" "$DOCKER_CONTEXT_ARGUMENT" \
        "$IMAGE_TAG" "$FLATTEN_IMAGE" \
        < "$DOCKERFILE_PATH" || return 1
    
    # Stop the secret variables server, if it's still running:
    echo "PUT /shut-down" > "$SECRET_SERVER_SOCKET_PATH"

    rm -R "$DOCKER_BUILD_DIR"
}
