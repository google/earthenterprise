#!/bin/bash

# Some of the Gauge tests cannot clean up after themselves because deleting
# assets requires superuser permissions. This script can be used to clean up
# after the tests instead.
export ASSETROOT=/gevol/assets
sudo rm -rf ${ASSETROOT}/Databases/Database*
sudo rm -rf ${ASSETROOT}/MapLayers/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Imagery/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Terrain/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Vector/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Map/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/BlueMarble*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/i3SF15meter*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/USGSLanSat*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/SFHiRes*
sudo rm -rf ${ASSETROOT}/Resources/Terrain/GTopo_Database*
sudo rm -rf ${ASSETROOT}/Resources/Vector/CA_POIs_*
sudo /etc/init.d/gefusion restart

touch /opt/google/share/tutorials/fusion/Imagery/BadTestImage1.jp2
touch /opt/google/share/tutorials/fusion/Imagery/BadTestImage2.jp2


