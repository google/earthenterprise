# Input variable definitions for building and running Docker images for
# testing Cent OS 6.

# Use this file in a command like:
#
# ```BASH
# ( . config/centos-6-test.sh && ./bin/build-gee-image.sh )
# ```

export OS_DISTRIBUTION=centos-6
export STAGE_1_NAME=current-clone
export CLEAN_CLONE_GIT_USER=tst-ppenev
export CLEAN_CLONE_BRANCH="#547-rhel6-fusion-rpm"

export GEE_SERVER_PORT=8080
