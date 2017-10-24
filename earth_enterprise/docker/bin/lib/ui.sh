# Source this file to get the functions defined below.

function tst_docker_warn()
{
    local MESSAGE="$1"

    echo -n "$(tput setaf 0)$(tput setab 3)"
    echo
    printf "%-${COLUMNS}s" "  Warning: $MESSAGE"
    echo
    echo "$(tput sgr0)"
}

function tst_docker_err()
{
    local MESSAGE="$1"

    echo -n "$(tput setaf 7)$(tput setab 1)"
    echo
    printf "%-${COLUMNS}s" "  Error: $MESSAGE"
    echo
    echo "$(tput sgr0)"
}

function tst_docker_read_variable()
{
    local VAR_NAME="$1"
    local PROMPT="$2"
    local VAR_TYPE="$3"

    if [ "$VAR_TYPE" == "password" ]; then
        while true; do
            echo -n "$PROMPT: "
            read -r -s "$VAR_NAME"
            echo
            echo -n "Confirm: "
            read -r -s
            echo
            if [ "${!VAR_NAME}" == "$REPLY" ]; then
                RESULT="shred value"
                break
            else
                echo "Values didn't match. Try again."
            fi
        done
    else
        echo -n "$PROMPT: "
        read -r "$VAR_NAME"
    fi
}
