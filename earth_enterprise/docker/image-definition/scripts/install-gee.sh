#! /bin/bash

# This script installs Open Google Earth Enterprise Server and Fusion.
#
# It has to be run in the directory where the Open GEE repository has been
# cloned.

# Prepare the install package:

# Download tutorial data:
# cd earthenterprise/earth_enterprise/tutorial && \
# mkdir -p FusionTutorial && \
# wget http://data.opengee.org/FusionTutorial-Full.tar.gz && \
# tar -xvzf FusionTutorial-Full.tar.gz -C FusionTutorial || exit 1

# Copy binaries to install package location:
# cd .. && \
cd earthenterprise/earth_enterprise && \
scons -j8 release=1 stage_install || exit 1

# Install Fusion:
cd src/installer &&
(echo C; echo C) | ./install_fusion.sh -hnmf || exit 1

# Install Earth Server:
(echo n) | ./install_server.sh -hnmf || exit 1

# Reload PATH:
source /etc/profile || exit 1
