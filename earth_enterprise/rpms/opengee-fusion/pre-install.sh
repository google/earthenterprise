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

#-----------------------------------------------------------------
BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"

# directory locations
BININSTALLROOTDIR="/etc/init.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"

# derived directories
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"
MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB=1048576

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"

#-----------------------------------------------------------------

# config values
ASSET_ROOT="/gevol/assets"
ASSET_ROOT_VOLUME_SIZE=0


#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
    service gefusion stop

    load_systemrc_config

    check_asset_root_volume_size

    # check for invalid asset names
    INVALID_ASSETROOT_NAMES=$(find $ASSET_ROOT -type d -name "*[\\\&\%\'\"\*\=\+\~\`\?\<\>\:\; ]*" 2> /dev/null)

    if [ ! -z "$INVALID_ASSETROOT_NAMES" ]; then
        show_invalid_assetroot_name $INVALID_ASSETROOT_NAMES
    fi
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_asset_root_volume_size()
{
    ASSET_ROOT_VOLUME_SIZE=$(df --output=avail $ASSET_ROOT | grep -v Avail)
    
    if [[ $ASSET_ROOT_VOLUME_SIZE -lt MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB ]]; then
        MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB=$(expr $MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB / 1024 / 1024)

        echo -e "\nThe asset root volume [$ASSET_ROOT] has only $ASSET_ROOT_VOLUME_SIZE KB available."
        echo -e "We recommend that an asset root directory have a minimum of $MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB GB of free disk space."
        echo ""
    fi
}

load_systemrc_config()
{
    if [ -f "$SYSTEMRC" ]; then
        # read existing config information
        ASSET_ROOT=$(xmllint --xpath '//Systemrc/assetroot/text()' $SYSTEMRC)
        GEFUSIONUSER_NAME=$(xmllint --xpath '//Systemrc/fusionUsername/text()' $SYSTEMRC)
        GROUPNAME=$(xmllint --xpath '//Systemrc/userGroupname/text()' $SYSTEMRC)		
    fi
}

show_invalid_assetroot_name()
(
    echo -e "\nThe following characters are no longer allowed in GEE Fusion Assets:"
    echo -e "& % \' \" * = + ~ \` ? < > : ; and the space character.\n"
    
    echo -e "Assets with these names will no longer be usable in GEE Fusion and will generate"
    echo -e "an appropriate error message.\n"

    echo -e "If you continue with installation, you will have to either recreate those assets using"
    echo -e "valid names or  rename the assets to a valid name before attempting to process the assets.\n"

    echo -e "See the GEE Admin Guide for more details on renaming assets.\n"

    echo -e "The following assets contain invalid characters:\n"
    echo -e "$1"
)


#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall "$@"
