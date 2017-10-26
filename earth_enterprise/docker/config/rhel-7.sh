# Input variable definitions for building and running Docker images for
# testing RHEL 7.

# Use this file in a command like:
#
# ```BASH
# ( . config/rhel7.sh && ./bin/build-gee-image.sh )
# ```

export OS_DISTRIBUTION=rhel-7
export STAGE_1_NAME=clean-clone
#export REDHAT_USERNAME=put-your-username-here
#export REDHAT_PASSWORD="your password here"
#export CLEAN_CLONE_GIT_USER=tst-ppenev
#export CLEAN_CLONE_BRANCH="#355-gtest_for_gee_on_rhel_6"

export GEE_SERVER_PORT=8080

# To run a custom image, e.g., with tutorial resources:
#export GEE_IMAGE_NAME=opengee-experimental-rhel-7-tutorial-resources

# Add custom `docker run` options, e.g. to use tutorial resources from the
# host file system:
#export DOCKER_RUN_FLAGS="-v /opt/google/share/tutorials/fusion/:/opt/google/share/tutorials/fusion/"
