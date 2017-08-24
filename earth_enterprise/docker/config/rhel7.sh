# Input variable definitions for building and running Docker images for
# testing RHEL 7.

# Use this file in a command like:
#
# ```BASH
# ( . config/rhel7.sh && ./bin/build-docker-image.sh )
# ```

export OS_DISTRIBUTION=rhel-7
export STAGE_1_NAME=clean-clone
#export REDHAT_USERNAME=put-your-username-here
#export REDHAT_PASSWORD="your password here"

export GEE_SERVER_PORT=8080
