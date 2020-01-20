#!/bin/bash

ASSETROOT=/gevol/assets
SRC_VOLUME=/gevol/src 
TUTORIAL_PATH=/opt/google/share/tutorials/fusion
TEST_DATA_PATH=${SRC_VOLUME}/gauge_tests
export PATH=${PATH}:/opt/google/bin

# Clean up from any previous tests
./test_teardown.sh

# Script will only download if necessary
sudo sh ${TUTORIAL_PATH}/download_tutorial.sh
sudo rsync -arv ${TUTORIAL_PATH}/ ${TEST_DATA_PATH}

# Copy template files so that gefusionuser can access them
sudo cp resources/* /gevol/src/gauge_tests

# Create a failed imagery resource
FAILED_RESOURCE_NAME="gauge_tests/Resources/Imagery/StatePropagationTest_FailedImageryResource"
FAILED_RESOURCE_ASSET_PATH=${ASSETROOT}/${FAILED_RESOURCE_NAME}.kiasset
genewimageryresource -o ${FAILED_RESOURCE_NAME} ${TEST_DATA_PATH}/Imagery/usgsSFHiRes.tif
gebuild ${FAILED_RESOURCE_NAME}
echo "Waiting for test resources to build: ${FAILED_RESOURCE_NAME}"
WAIT_COUNT=0
while [ `gequery ${FAILED_RESOURCE_NAME} --status` != "Succeeded" ] && [[ ${WAIT_COUNT} -lt 60 ]]; do
  sleep 1
  (( WAIT_COUNT++ ))
done
# Mark ALL versions as failed
find ${FAILED_RESOURCE_ASSET_PATH} -name "khassetver.xml" -exec sudo sed -i.bak "s/<state>Succeeded/<state>Failed/g" {} \;
if [ `gequery ${FAILED_RESOURCE_NAME} --status` != "Failed" ]; then
  echo "Error setting up tests. Please try again."
fi

# Allow modifying misc.xml during tests
sudo chmod o+w /gevol/assets/.config/misc.xml

sudo /etc/init.d/gefusion restart
