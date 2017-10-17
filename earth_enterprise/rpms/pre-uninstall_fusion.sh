#!/bin/bash
#
# Copyright 2017 Google Inc.
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

# NOTE: requires xmllint from libxml2-utils

set +x

# get script directory
# config values
ASSET_ROOT=""
GEFUSIONUSER_NAME=""
GROUPNAME=""

# directory locations
BININSTALLROOTDIR="/etc/init.d"

# script arguments
HAS_EARTH_SERVER=false

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preuninstall()
{
    service gefusion stop

    load_systemrc_config

    GROUP_EXISTS=$(getent group $GROUPNAME)
    USERNAME_EXISTS=$(getent passwd $GEFUSIONUSER_NAME)

    if [ -f "$BININSTALLROOTDIR/geserver" ]; then
        HAS_EARTH_SERVER=true
    else
        HAS_EARTH_SERVER=false
    fi

    verify_systemrc_config_values
}

#-----------------------------------------------------------------
# Pre-uninstall Functions
#-----------------------------------------------------------------
load_systemrc_config()
{
    if [ -f "$SYSTEMRC" ]; then
        ASSET_ROOT=$(xmllint --xpath '//Systemrc/assetroot/text()' $SYSTEMRC)
	GEFUSIONUSER_NAME=$(xmllint --xpath '//Systemrc/fusionUsername/text()' $SYSTEMRC)
	GROUPNAME=$(xmllint --xpath '//Systemrc/userGroupname/text()' $SYSTEMRC)	
    else
        echo -e "\nThe system configuration file [$SYSTEMRC] could not be found on your system."
        echo -e "This uninstaller cannot continue without a valid system configuration file.\n"
    fi
}


verify_systemrc_config_values()
{
    # now let's make sure that the config values read from systemrc each contain data
    if [ -z "$ASSET_ROOT" ] || [ -z "$GEFUSIONUSER_NAME" ] || [ -z "$GROUPNAME" ]; then
        echo -e "\nThe [$SYSTEMRC] configuration file contains invalid data."
        echo -e "\nAsset Root: \t\t$ASSET_ROOT"
        echo -e "Fusion User: \t$GEFUSIONUSER_NAME"
        echo -e "Fusion Group: \t\t$GROUPNAME"
        echo -e "\nThe uninstaller requires a system configuration file with valid data."
    fi
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------
main_preuninstall "$@"
