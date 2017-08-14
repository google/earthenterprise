#! /bin/bash

SELF_NAME=$(basename "$0")

CLONE_URL=""
CLONE_BRANCH=""

function show_usage()
{
    cat <<USAGE
$SELF_NAME [-b|--branch <branch-name>] [<repository-url>]

    <repository-url>            URL of the repository to clone.
    -b|--branch <branch-name>   The name of the Git branch to check out.
USAGE
}

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -b|--branch)
            CLONE_BRANCH="$2"
            shift
            ;;
        *)
            if [ -n "$CLONE_URL" ]; then
                echo "Unrecognized command-line argument: $1" >&2
                echo "----------------------------------" >&2
                echo
                show_usage
                exit 1
            fi
            CLONE_URL="$1"
            ;;
    esac
    shift # Consume the command-line argument being parsed.
done

: ${CLONE_URL:="https://github.com/google/earthenterprise.git"}

if [ -d earthenterprise -a -d earthenterprise/.git ]; then
    echo "The GEE Open Source repository has already been cloned. Pulling latest updates"
    cd earthenterprise
    if [ -n "$CLONE_BRANCH" ]; then
        git checkout "$CLONE_BRANCH"
    fi
    git pull
    git lfs pull
else
    if [ -z "$CLONE_BRANCH" ]; then
        GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 "$CLONE_URL"
    else
        GIT_LFS_SKIP_SMUDGE=1 git clone -b "$CLONE_BRANCH" --depth 1 "$CLONE_URL"
    fi
    cd earthenterprise
    git lfs pull
fi
