#!/bin/bash

# Change the directory to the location where the main script is located.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR

./balaOutputPathTest.sh

./balaSingleBoundaryValidationEngine.sh north_boundary
./balaSingleBoundaryValidationEngine.sh south_boundary
./balaSingleBoundaryValidationEngine.sh east_boundary
./balaSingleBoundaryValidationEngine.sh west_boundary
