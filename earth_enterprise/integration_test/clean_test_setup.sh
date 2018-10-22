#!/bin/bash

# Some of the Gauge tests cannot clean up after themselves because deleting
# assets requires superuser permissions. This script can be used to clean up
# after the tests instead.
export ASSETROOT=/gevol/assets
export SRC_VOLUME=/gevol/src 
export TUTORIAL_PATH=/opt/google/share/tutorials/fusion
export TEST_DATA_PATH=${SRC_VOLUME}/gauge_tests
export PATH=${PATH}:/opt/google/bin

sudo service gefusion stop
sudo rm -rf ${ASSETROOT}/Databases/Database*
sudo rm -rf ${ASSETROOT}/Databases/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/MapLayers/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Imagery/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Terrain/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Vector/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Projects/Map/StatePropagationTest*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/BlueMarble*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/i3SF15meter*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/USGSLanSat*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/BadImage*
sudo rm -rf ${ASSETROOT}/Resources/Imagery/SFHiRes*
sudo rm -rf ${ASSETROOT}/Resources/Terrain/GTopo_Database*
sudo rm -rf ${ASSETROOT}/Resources/Vector/CA_POIs_* 
sudo chown -R gefusionuser:gegroup ${ASSETROOT}
sudo service gefusion start

# Script will only download if necessary
sh ${TUTORIAL_PATH}/download_tutorial.sh
rm -rf ${TEST_DATA_PATH}/*
rsync -arv ${TUTORIAL_PATH}/ ${TEST_DATA_PATH}


# create a failed imagery resource
export FAILED_RESOURCE_NAME="Resources/Imagery/StatePropagationTest_FailedImageryResource"
export FAILED_RESOURCE_ASSET_PATH=${ASSETROOT}/${FAILED_RESOURCE_NAME}.kiasset
genewimageryresource -o ${FAILED_RESOURCE_NAME} ${TEST_DATA_PATH}/Imagery/usgsSFHiRes.tif
gebuild ${FAILED_RESOURCE_NAME}
echo "Sleeping for 60 seconds, waiting for test resources to build: ${FAILED_RESOURCE_NAME}"
sleep 60
gequery ${FAILED_RESOURCE_NAME} --status
#Mark ALL versions as failed
find ${FAILED_RESOURCE_ASSET_PATH} -name "khassetver.xml" -exec sed -i.bak "s/<state>Succeeded/<state>Failed/g" {} \;
gequery ${FAILED_RESOURCE_NAME} --status