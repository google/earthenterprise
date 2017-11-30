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
# Pushes databases built by run_tutorial.sh script
set -x
set -e

ASSET_ROOT="/usr/local/google/gevol_test/assets"
echo "Using asset root: $ASSET_ROOT"

/opt/google/bin/geserveradmin --adddb "$ASSET_ROOT/Tutorial/Databases/SFinset_1.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --pushdb "$ASSET_ROOT/Tutorial/Databases/SFinset_1.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --publishdb "$ASSET_ROOT/Tutorial/Databases/SFinset_1.kdatabase/gedb.kda/ver001/gedb" --targetpath /SFinset_1

/opt/google/bin/geserveradmin --adddb "$ASSET_ROOT/Tutorial/Databases/SFinset_2.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --pushdb "$ASSET_ROOT/Tutorial/Databases/SFinset_2.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --publishdb "$ASSET_ROOT/Tutorial/Databases/SFinset_2.kdatabase/gedb.kda/ver001/gedb" --targetpath /SFinset_2

/opt/google/bin/geserveradmin --adddb "$ASSET_ROOT/Tutorial/Databases/SFinset_3.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --pushdb "$ASSET_ROOT/Tutorial/Databases/SFinset_3.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --publishdb "$ASSET_ROOT/Tutorial/Databases/SFinset_3.kdatabase/gedb.kda/ver001/gedb" --targetpath /SFinset_3

/opt/google/bin/geserveradmin --adddb "$ASSET_ROOT/Tutorial/Databases/SFinset_4.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --pushdb "$ASSET_ROOT/Tutorial/Databases/SFinset_4.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --publishdb "$ASSET_ROOT/Tutorial/Databases/SFinset_4.kdatabase/gedb.kda/ver001/gedb" --targetpath /SFinset_4

/opt/google/bin/geserveradmin --adddb "$ASSET_ROOT/Tutorial/Databases/SFinset_5.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --pushdb "$ASSET_ROOT/Tutorial/Databases/SFinset_5.kdatabase/gedb.kda/ver001/gedb"
/opt/google/bin/geserveradmin --publishdb "$ASSET_ROOT/Tutorial/Databases/SFinset_5.kdatabase/gedb.kda/ver001/gedb" --targetpath /SFinset_5
