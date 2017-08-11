#! /bin/bash

SELF_NAME=$(basename "$0")

CLONE_BRANCH=""

function show_usage()
{
    cat <<USAGE
$SELF_NAME [-b|--branch <branch-name>]

    -b|--branch <branch-name>   The name of the Git branch to clone.
USAGE
}

while [[ "$#" -gt 1 ]]; do
    case "$1" in
        -b|--branch)
            CLONE_BRANCH="$2"
            shift
            ;;
        *)
            echo "Unrecognized command-line argument: $1" >&2
            echo "----------------------------------" >&2
            echo
            show_usage
            exit 1
            ;;
    esac
    shift # Consume the command-line argument being parsed.
done

if [ -d earthenterprise -a -d earthenterprise/.git ]; then
    echo "The GEE Open Source repository has already been cloned. Pulling latest updates"
    cd earthenterprise
    if [ -n "CLONE_BRANCH" ]; then
        git checkout "$CLONE_BRANCH"
    fi
    git pull
    git lfs pull
else
    GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/google/earthenterprise.git
    cd earthenterprise
    if [ -n "CLONE_BRANCH" ]; then
        git checkout "$CLONE_BRANCH"
    fi
    git lfs pull
fi
