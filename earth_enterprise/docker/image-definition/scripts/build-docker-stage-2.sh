#! /bin/bash

SELF_DIR=$(dirname "$0")

cd "$SELF_DIR/../build"

# Install Open GEE.
#
# We have to do this outside the Dockerfile, because geconfigureassetroot,
# which is run during installation, requires the 'DAC_READ_SEARCH' capability,
# and Docker currently has no way of running Dockerfile commands with
# capabilities.
/root/opengee/bin/install-gee.sh || exit 1

# Install Portable Server:
/root/opengee/bin/install-portable-server.sh || exit 1

# Remove the GEE repository and build files to save space:
/root/opengee/bin/rm-gee-build-files.sh || exit 1

# Clean the package cache to save space:
if type apt-get >/dev/null 2>&1; then
    apt-get clean
else
    yum clean all
fi
