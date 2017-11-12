#! /bin/bash

# This script installs Open Google Earth Enterprise Server and Fusion.
#
# It has to be run in the directory where the Open GEE repository has been
# cloned, and after Open GEE and Fusion have been built and staged for
# install.

# Tests whether a glob matches any files.
#
# E.g.:
#
#   glob_matches *.beef && echo "Dinner."
glob_matches()
{
    [ -e "$1" ]
}

PACKAGES_DIR="earthenterprise/earth_enterprise/rpms/build/distributions"



if type apt-get >/dev/null 2>&1 && glob_matches "$PACKAGES_DIR"/*.deb; then
    # Use Deb packages:
    cd "$PACKAGES_DIR"
    dpkg -GEi *.deb
elif type yum >/dev/null 2>&1 && glob_matches "$PACKAGES_DIR"/*.rpm; then
    # Use RPM packages:
    cd "$PACKAGES_DIR"
    yum install -y *.rpm
else
    echo "$(basename "$0"): Using the old GEE Server and Fusion installers."

    # Install Fusion:
    cd earthenterprise/earth_enterprise/src/installer || exit 1
    (echo C; echo C) | ./install_fusion.sh -hnmf || exit 1

    # Install Earth Server:
    (echo n) | ./install_server.sh -hnmf || exit 1
fi

# Reload PATH:
source /etc/profile || exit 1
