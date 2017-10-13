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
DELETE_FUSION_GROUP=true

HAS_EARTH_SERVER=false

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preuninstall()
{
    # check to see if GE Fusion processes are running
    if ! check_fusion_processes_running; then
        # STOP PROCESSES
    fi

    load_systemrc_config

    GROUP_EXISTS=$(getent group $GROUPNAME)
    USERNAME_EXISTS=$(getent passwd $GEFUSIONUSER_NAME)

    if [ -f "$BININSTALLROOTDIR/geserver" ]; then
        HAS_EARTH_SERVER=true
    else
        HAS_EARTH_SERVER=false
    fi

    verify_systemrc_config_values

    verify_group
}

#-----------------------------------------------------------------
# Pre-uninstall Functions
#-----------------------------------------------------------------
check_fusion_processes_running()
{
	check_fusion_processes_running_retval=0

	local manager_running=$(ps -e | grep gesystemmanager | grep -v grep)
	local res_provider_running=$(ps -ef | grep geresourceprovider | grep -v grep)

	if [ ! -z "$manager_running" ] || [ ! -z "$res_provider_running" ]; then
		check_fusion_processes_running_retval=1
	fi

	return $check_fusion_processes_running_retval
}

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

verify_group()
{
    if [ $DELETE_FUSION_GROUP == true ] && [ $HAS_EARTH_SERVER == true ]; then
        echo -e "\nNote: the GEE group [$GROUPNAME] will not be deleted because $GEES is installed on"
        echo -e "this server. $GEES uses this account too."
        echo -e "The group account will be deleted when $GEES is uninstalled."
        DELETE_FUSION_GROUP=false
    fi
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------
main_preuninstall "$@"
