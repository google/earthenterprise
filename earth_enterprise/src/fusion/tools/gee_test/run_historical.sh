#!/bin/bash -eu
#
# Copyright 2017 Open GEE Contributors
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
#
# Runs most commands referenced in the Fusion tutorial. Handy for doing
# testing.

set -x
set -e

ASSET_ROOT="/usr/local/google/gevol_test/assets"
echo "Using asset root: $ASSET_ROOT"

# This is a simplified tutorial script.  It does not have all the dependencies as the Python QA tutorial script, nor does it do any kind of checking.
: '
'
# - Copy needed files


# - Fusion Commands

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2012-05-10T12:00:00Z" --feather 100 --masktolerance 2 -o Tutorial/Resources/Imagery/SFBayAreaLanSat_x /opt/google/share/tutorials/fusion/Imagery/usgsLanSat.tif

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2012-05-12T12:00:00Z" --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_1 /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif
/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2012-05-10T14:00:00Z" --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_2 /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif
/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2012-05-10T12:10:10Z" --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_3 /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif
/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2012-10-10T12:10:10Z" --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_4 /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif
/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate "2017-05-10T12:10:10Z" --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_5 /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif

/opt/google/bin/genewimageryproject --historical_imagery -o Tutorial/Projects/Imagery/SFinset_1 Tutorial/Resources/Imagery/SFBayAreaLanSat_x Tutorial/Resources/Imagery/SFHighResInset_1 Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/i3_15Meter
/opt/google/bin/genewimageryproject --historical_imagery -o Tutorial/Projects/Imagery/SFinset_2 Tutorial/Resources/Imagery/SFBayAreaLanSat_x Tutorial/Resources/Imagery/SFHighResInset_2 Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/i3_15Meter
/opt/google/bin/genewimageryproject --historical_imagery -o Tutorial/Projects/Imagery/SFinset_3 Tutorial/Resources/Imagery/SFBayAreaLanSat_x Tutorial/Resources/Imagery/SFHighResInset_3 Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/i3_15Meter
/opt/google/bin/genewimageryproject --historical_imagery -o Tutorial/Projects/Imagery/SFinset_4 Tutorial/Resources/Imagery/SFBayAreaLanSat_x Tutorial/Resources/Imagery/SFHighResInset_4 Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/i3_15Meter
/opt/google/bin/genewimageryproject --historical_imagery -o Tutorial/Projects/Imagery/SFinset_5 Tutorial/Resources/Imagery/SFBayAreaLanSat_x Tutorial/Resources/Imagery/SFHighResInset_5 Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/i3_15Meter

/opt/google/bin/genewdatabase -o Tutorial/Databases/SFinset_1 --imagery Tutorial/Projects/Imagery/SFinset_1
/opt/google/bin/genewdatabase -o Tutorial/Databases/SFinset_2 --imagery Tutorial/Projects/Imagery/SFinset_2
/opt/google/bin/genewdatabase -o Tutorial/Databases/SFinset_3 --imagery Tutorial/Projects/Imagery/SFinset_3
/opt/google/bin/genewdatabase -o Tutorial/Databases/SFinset_4 --imagery Tutorial/Projects/Imagery/SFinset_4
/opt/google/bin/genewdatabase -o Tutorial/Databases/SFinset_5 --imagery Tutorial/Projects/Imagery/SFinset_5

/opt/google/bin/gebuild Tutorial/Databases/SFinset_1
/opt/google/bin/gebuild Tutorial/Databases/SFinset_2
/opt/google/bin/gebuild Tutorial/Databases/SFinset_3
/opt/google/bin/gebuild Tutorial/Databases/SFinset_4
/opt/google/bin/gebuild Tutorial/Databases/SFinset_5
