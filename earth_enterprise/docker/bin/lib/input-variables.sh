# Source this file to get functions defined below.

SELF_PATH="${BASH_SOURCE}"
SELF_DIR=$(dirname "$SELF_PATH")

source "$SELF_DIR/image-naming.sh"

function build_image_parse_input_variables()
{
    # Set this to the OS distribution that Earth Server, Fusion and Portable
    # Server are to be built on.
    : ${OS_DISTRIBUTION:="ubuntu-14"}

    # Set this to a stage 1 template name (e.g. 'clean-clone' for the
    # 'Dockerfile.stage-1.clean-clone.template').  The 'clean-clone' template
    # clones from the Git Hub repository before building.  The 'current-clone'
    # template copies the current files in your development repository.
    : ${STAGE_1_NAME:="clean-clone"}

    # Set this to the URL of the repository to clone during a "clean-clone"
    # stage 1.  This variable is not used for "current-clone" stage 1.  If left
    # blank, a URL will be derived from the CLEAN_CLONE_GIT_USER value, or the
    # default GEE Git URL will be used.
    : ${CLEAN_CLONE_URL:=""}

    # Instead of setting the full CLEAN_CLONE_URL, you can set this to the
    # username, if you want to build from a Git user's fork of the GEE
    # repository.  Git forks use the same repository name as the upstream, so
    # a username is enough to construct the repository URL.
    : ${CLEAN_CLONE_GIT_USER:=""}

    if [ -z "$CLEAN_CLONE_URL" -a -n "$CLEAN_CLONE_GIT_USER" ]; then
        CLEAN_CLONE_URL="https://github.com/${CLEAN_CLONE_GIT_USER}/earthenterprise.git"
    fi

    # Set this to the name of a branch that you want to build during a
    # "clean-clone" stage 1.  This variable is not used for the "current-clone"
    # stage 1.
    : ${CLEAN_CLONE_BRANCH:=""}

    CLONE_SUFFIX=$(tst_docker_naming_echo_clone_suffix "$STAGE_1_NAME" "$CLEAN_CLONE_URL" "$CLEAN_CLONE_BRANCH")

    : ${GEE_IMAGE_NAME:="opengee-experimental-${OS_DISTRIBUTION}${CLONE_SUFFIX}"}
}