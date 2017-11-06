#! /bin/bash

# This script installs Open Google Earth Enterprise Server and Fusion.
#
# It has to be run in the directory where the Open GEE repository has been
# cloned.

# Prepare the install package:

# Copy binaries to install package location:
# cd earthenterprise/earth_enterprise || exit 1
# scons -j8 release=1 stage_install || exit 1



if type apt-get >/dev/null 2>&1; then
    # Use RPM packages:
    cd earthenterprise/earth_enterprise/rpms/build/distributions
    dpkg -GEi *.deb
elif type yum >/dev/null 2>&1; then
    # Use Deb packages:
    cd earthenterprise/earth_enterprise/rpms/build/distributions
    yum install -y *.rpm
else
    # Use the old installers:
    
    # Install Fusion:
    cd earthenterprise/earth_enterprise/src/installer || exit 1
    (echo C; echo C) | ./install_fusion.sh -hnmf || exit 1

    # Install Earth Server:
    (echo n) | ./install_server.sh -hnmf || exit 1
fi

# Reload PATH:
source /etc/profile || exit 1
