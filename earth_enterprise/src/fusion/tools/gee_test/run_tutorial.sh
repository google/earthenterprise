#!/bin/bash -eu
#
# Copyright 2017 Google Inc.
# Copyright 2018 Open GEE Authors
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
TODO: check for current assetroot and manage to work with "gevol_test"- assetroot.
 IF current assetroot is $ASSET_ROOT
 THEN
    - back up postgres databases and report backup folder;
    - reset postgres databases;
    - delete Tutorial/Databases/Tutorial/Resources/Projects folders from $ASSET_ROOT (need sudo)
ELSE  # not gevol_test
    - back up postgres databases and report back up folder;
    - report/store current(source) assetroot path/published_dbs path;
    - create gevol_test assetroot $ASSET_ROOT;
    - create gevol_test/published_dbs
    - switch to gevol_test assetroot/published_dbs;

...run tests

Note: This might be optional since we may want to test serving of published gee_test Databases.
Maybe better just report command lines to restore user assetroot and postgres databases.
- Back up gevol_test postgres databases;
IF source assetroot was not gevol_test;
   - Switch to source assetroot/published_dbs
   - restore postgres database;

Also we may consider to create assetroot for test with timestamp ID to have it unique and to not accidentally delete user assetroot which may be gevol_test.

'
# - Copy needed files
sudo mkdir -p $ASSET_ROOT/.userdata/
sudo chmod 777 $ASSET_ROOT/.userdata/
sudo cp tutorial_files/providers.xml $ASSET_ROOT/.userdata/
sudo chmod 666 $ASSET_ROOT/.userdata/providers.xml
mkdir -p /tmp/tutorial
cp tutorial_files/CA_Freeways_MapLayer.kmdsp /tmp/tutorial/
cp tutorial_files/USA_Counties_MapLayer.kmdsp /tmp/tutorial/
cp tutorial_files/CA_Highways_Full_Layer.khdsp /tmp/tutorial/
chmod -R 755 /tmp/tutorial/

sudo /etc/init.d/gefusion restart
sudo /etc/init.d/geserver restart


# - Fusion Commands

/opt/google/bin/genewimageryresource --nomask --provider NASA --sourcedate 2000-05-10 -o Tutorial/Resources/Imagery/BlueMarble /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate 2003-05-10 --feather 100 --masktolerance 2 -o Tutorial/Resources/Imagery/SFBayAreaLanSat /opt/google/share/tutorials/fusion/Imagery/usgsLanSat.tif

/opt/google/bin/genewimageryresource --provider I3 --sourcedate 2006-05-10 --feather 100 -o Tutorial/Resources/Imagery/i3_15Meter /opt/google/share/tutorials/fusion/Imagery/i3SF15-meter.tif

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate 2008-05-01 --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif

/opt/google/bin/genewimageryresource --nomask --provider NASA --sourcedate 2000-05-10 -o Tutorial/Resources/Imagery/BlueMarble_merc /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif --mercator

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate 2003-05-10 --feather 100 --masktolerance 2 -o Tutorial/Resources/Imagery/SFBayAreaLanSat_merc /opt/google/share/tutorials/fusion/Imagery/usgsLanSat.tif --mercator

/opt/google/bin/genewimageryresource --provider I3 --sourcedate 2006-05-10 --feather 100 -o Tutorial/Resources/Imagery/i3_15Meter_merc /opt/google/share/tutorials/fusion/Imagery/i3SF15-meter.tif --mercator

/opt/google/bin/genewimageryresource --provider USGS-I --sourcedate 2008-05-01 --feather 100 -o Tutorial/Resources/Imagery/SFHighResInset_merc /opt/google/share/tutorials/fusion/Imagery/usgsSFHiRes.tif --mercator

/opt/google/bin/genewterrainresource --scale 1 --provider USGS-T --sourcedate 2003-05-10 --mosaictolerance 0 --feather 100 --holesize 0 -o Tutorial/Resources/Terrain/SFTerrain /opt/google/share/tutorials/fusion/Terrain/SF_terrain.tif

/opt/google/bin/genewterrainresource --scale 1 --provider USGS-T --sourcedate 2003-05-10 --havemask -o Tutorial/Resources/Terrain/Topo30 /opt/google/share/tutorials/fusion/Terrain/gtopo30_4km.tif

/opt/google/bin/genewvectorresource --sourcedate 2003-05-10 --encoding UTF-8 --layer 0 --provider USGS-M -o Tutorial/Resources/Vector/CAHighways /opt/google/share/tutorials/fusion/Vector/california_roads_line.shp

/opt/google/bin/genewvectorresource --sourcedate 2003-05-10 --encoding UTF-8 --layer 0 --provider USGS-P -o Tutorial/Resources/Vector/CA_POIs /opt/google/share/tutorials/fusion/Vector/california_popplaces.csv

/opt/google/bin/genewvectorresource --sourcedate 2003-05-10 --encoding UTF-8 --layer 0 --provider GNIS -o Tutorial/Resources/Vector/US_Population /opt/google/share/tutorials/fusion/Vector/us_counties_census.shp

/opt/google/bin/genewvectorresource --sourcedate 2003-05-10 --encoding UTF-8 --layer 0 --provider GNIS -o Tutorial/Resources/Vector/CA_County_Population /opt/google/share/tutorials/fusion/Vector/cal_counties_census.shp

/opt/google/bin/genewmaplayer --legend=CA_Roads  --default_state_on --output=Tutorial/MapLayers/CaRoads --layericon=road --template=/tmp/tutorial/CA_Freeways_MapLayer.kmdsp Tutorial/Resources/Vector/CAHighways

/opt/google/bin/genewmaplayer --legend=US_Population  --default_state_on --output=Tutorial/MapLayers/US_Population --layericon=picnic --template=/tmp/tutorial/USA_Counties_MapLayer.kmdsp Tutorial/Resources/Vector/US_Population

/opt/google/bin/gebuild Tutorial/Resources/Vector/CAHighways

/opt/google/bin/gebuild Tutorial/Resources/Vector/CA_POIs

/opt/google/bin/gebuild Tutorial/Resources/Vector/US_Population

/opt/google/bin/gebuild Tutorial/Resources/Vector/CA_County_Population

/opt/google/bin/gebuild Tutorial/MapLayers/CaRoads

/opt/google/bin/gebuild Tutorial/MapLayers/US_Population

/opt/google/bin/genewimageryproject -o Tutorial/Projects/Imagery/SFBayArea Tutorial/Resources/Imagery/BlueMarble Tutorial/Resources/Imagery/SFBayAreaLanSat Tutorial/Resources/Imagery/i3_15Meter Tutorial/Resources/Imagery/SFHighResInset

/opt/google/bin/genewimageryproject -o Tutorial/Projects/Imagery/SFBayArea_merc --mercator Tutorial/Resources/Imagery/BlueMarble_merc Tutorial/Resources/Imagery/SFBayAreaLanSat_merc Tutorial/Resources/Imagery/i3_15Meter_merc Tutorial/Resources/Imagery/SFHighResInset_merc

/opt/google/bin/genewterrainproject -o Tutorial/Projects/Terrain/SFTerrain Tutorial/Resources/Terrain/SFTerrain

/opt/google/bin/genewterrainproject -o Tutorial/Projects/Terrain/SFTopo30Terrain Tutorial/Resources/Terrain/SFTerrain Tutorial/Resources/Terrain/Topo30

/opt/google/bin/genewvectorproject -o Tutorial/Projects/Vector/CA_Projects --template /tmp/tutorial/CA_Highways_Full_Layer.khdsp Tutorial/Resources/Vector/CAHighways

/opt/google/bin/genewmapproject -o Tutorial/Projects/Maps/CAProjects "Tutorial/MapLayers/CaRoads --legend=CA_Roads"

/opt/google/bin/genewdatabase -o Tutorial/Databases/SFDb_3d --imagery Tutorial/Projects/Imagery/SFBayArea --terrain Tutorial/Projects/Terrain/SFTopo30Terrain  --vector Tutorial/Projects/Vector/CA_Projects

/opt/google/bin/gebuild Tutorial/Databases/SFDb_3d

/opt/google/bin/genewmapdatabase --mercator -o Tutorial/Databases/SF_2d_Merc --imagery Tutorial/Projects/Imagery/SFBayArea_merc --map Tutorial/Projects/Maps/CAProjects

/opt/google/bin/genewmapdatabase --mercator -o Tutorial/Databases/SF_2d_Merc_With_Flat_Imagery --imagery Tutorial/Projects/Imagery/SFBayArea --map Tutorial/Projects/Maps/CAProjects

/opt/google/bin/gebuild Tutorial/Databases/SF_2d_Merc

/opt/google/bin/gebuild Tutorial/Databases/SF_2d_Merc_With_Flat_Imagery

set +x

echo "Check status with:"
echo "  /opt/google/bin/gequery Tutorial/Databases/SFDb_3d --status"
echo "Once 'Succeeded', run:"
echo "  /opt/google/bin/geserveradmin --adddb $ASSET_ROOT/Tutorial/Databases/SFDb_3d.kdatabase/gedb.kda/ver001/gedb/"
echo "  /opt/google/bin/geserveradmin --pushdb $ASSET_ROOT/Tutorial/Databases/SFDb_3d.kdatabase/gedb.kda/ver001/gedb/"
echo ""
echo "Check status with:"
echo "  /opt/google/bin/gequery Tutorial/Databases/SF_2d_Merc --status"
echo "Once 'Succeeded', run:"
echo "  /opt/google/bin/geserveradmin --adddb $ASSET_ROOT/Tutorial/Databases/SF_2d_Merc.kmmdatabase/mapdb.kda/ver001/mapdb"
echo "  /opt/google/bin/geserveradmin --pushdb $ASSET_ROOT/Tutorial/Databases/SF_2d_Merc.kmmdatabase/mapdb.kda/ver001/mapdb"
echo ""
echo " The same for all other databases..."

/opt/google/bin/gequery Tutorial/Databases/SFDb_3d --status
/opt/google/bin/gequery Tutorial/Databases/SF_2d_Merc --status
/opt/google/bin/gequery Tutorial/Databases/SF_2d_Merc_With_Flat_Imagery --status
