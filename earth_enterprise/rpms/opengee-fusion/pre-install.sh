#! /bin/bash
#
# Copyright 2017 the Open GEE Contributors
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
# Versions and user names:
GEE="Google Earth Enterprise"
#-----------------------------------------------------------------

# config values
ASSET_ROOT_VOLUME_SIZE=0


#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
    if test -f /etc/init.d/gefusion ; then
        service gefusion stop ; fi

    check_username $GEFUSIONUSER

# TODO: This seems broken...
#    load_systemrc_config

# TODO: depends on df option not found in rhel6/centos6
#    check_asset_root_volume_size

    # check for invalid asset names
    INVALID_ASSETROOT_NAMES=$(find "$ASSET_ROOT" -type d -name "*[\\\&\%\'\"\*\=\+\~\`\?\<\>\:\; ]*" 2> /dev/null)

    if [ -n "$INVALID_ASSETROOT_NAMES" ]; then
        show_invalid_assetroot_name "$INVALID_ASSETROOT_NAMES"
    fi
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_asset_root_volume_size()
{
    ASSET_ROOT_VOLUME_SIZE=$(df --output=avail "$ASSET_ROOT" | grep -v Avail)
    
    if [[ "$ASSET_ROOT_VOLUME_SIZE" -lt MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB ]]; then
        MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB=$(expr "$MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB" / 1024 / 1024)

        cat <<END

The asset root volume [$ASSET_ROOT] has only $ASSET_ROOT_VOLUME_SIZE KB available.
We recommend that an asset root directory have a minimum of $MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB GB of free disk space.

END
    fi
}

check_username() {
    USERNAME_EXISTS=$(getent passwd $1)

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p $BASEINSTALLDIR_OPT/.users/$1
        useradd --home $BASEINSTALLDIR_OPT/.users/$1 --system --gid $GEGROUP $1
    else
        # user already exists -- update primary group
        usermod -g $GEGROUP $1
    fi
}

show_invalid_assetroot_name()
(
    cat<<END

The following characters are no longer allowed in GEE Fusion Assets:
& % \' \" * = + ~ \` ? < > : ; and the space character.

Assets with these names will no longer be usable in GEE Fusion and will generate
an appropriate error message.

If you continue with installation, you will have to either recreate those assets using
valid names or  rename the assets to a valid name before attempting to process the assets.

See the GEE Admin Guide for more details on renaming assets.

The following assets contain invalid characters:

$1
END
)


#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall "$@"
