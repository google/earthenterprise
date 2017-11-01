#!/bin/bash

# Copyright 2017 the Open GEE Contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This is an overall runner for all of the Open GEE unit tests.  This will only
# run fusion unit tests if fusion has already been built.

# New tests should be added here as they are created.  To add a new test:
#   1. Add any code necessary to call the test script from this directory.
#   2. Call test_banner with the name of the test right before the test script
#      is called (it makes the output nicer).
#   3. Ensure that FAILURES is incremented if the test script fails.

FAILURES=0

BANNER="============================================================"

banner() {
  echo
  echo "$BANNER"
  echo "$1"
  echo "$BANNER"
  echo
}

test_banner() {
  banner "Running $1"
}

SCRIPT_DIR=`dirname "$0"`
BASE_DIR=$(cd "${SCRIPT_DIR}/../earth_enterprise"; pwd)

# Packaging (RPM and DEB) tests
test_banner "Packaging Unit Tests"
cd "${BASE_DIR}/rpms/test"
for TEST in `ls *_test.sh`; do
  ./"${TEST}" || ((FAILURES++))
done

# Fusion unit tests
SRC_DIR="${BASE_DIR}/src"
cd "${SRC_DIR}"
for BUILD_DIR in `ls -d NATIVE-* 2> /dev/null`; do
  test_banner "Fusion Unit Tests in ${BUILD_DIR}"
  cd "${SRC_DIR}/${BUILD_DIR}/bin/tests"
  ./RunAllTests.pl || ((FAILURES++))
done

# Report results
if [ $FAILURES -eq 0 ]; then
  banner "All test scripts passed."
elif [ $FAILURES -eq 1 ]; then
  banner "FAILURE: 1 test script failed."
else
  banner "FAILURE: $FAILURES test scripts failed."
fi

