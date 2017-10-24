# Source this file to get functions defined below.

function tst_docker_naming_echo_clone_suffix()
{
    local STAGE_1_NAME="$1"
    local CLEAN_CLONE_URL="$2"
    local CLEAN_CLONE_BRANCH="$3"
    local CLONE_SUFFIX=""
    local URL_SUFFIX
    local SANITIZED_BRANCH_NAME

    if [ "$STAGE_1_NAME" == "clean-clone" ]; then
        CLONE_SUFFIX=""
        if [ -n "$CLEAN_CLONE_URL" ]; then
            # Strip the URL down to "user":
            if [ "${CLEAN_CLONE_URL%://*}" == "$CLEAN_CLONE_URL" ]; then
                # It's a "git@github.com:user/repo.git" type of URL.
                URL_SUFFIX="${CLEAN_CLONE_URL#*:}"
            else
                # It's an "https://github.com/user/repo.git" type of URL.
                URL_SUFFIX="${CLEAN_CLONE_URL#*://}"
                URL_SUFFIX="${URL_SUFFIX#*/}"
            fi
            URL_SUFFIX="${URL_SUFFIX%.git}"
            URL_SUFFIX="${URL_SUFFIX%/*}"

            # Replace slashes with underscores:
            URL_SUFFX="${URL_SUFFIX//\//_}"

            if [ "$URL_SUFFIX" != "google" ]; then
                CLONE_SUFFIX="$CLONE_SUFFIX-$URL_SUFFIX"
            fi
        fi
        if [ -n "$CLEAN_CLONE_BRANCH" ]; then
            SANITIZED_BRANCH_NAME="$(echo -n "$CLEAN_CLONE_BRANCH" | tr '[:upper:]' '[:lower:]' |\
                tr --complement 'a-z0-9_.-' '_')"
            CLONE_SUFFIX="$CLONE_SUFFIX-branch-$SANITIZED_BRANCH_NAME"
        fi
    else
        CLONE_SUFFIX="-$STAGE_1_NAME"
    fi

    echo "$CLONE_SUFFIX"
}
