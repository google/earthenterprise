#! /bin/bash
#
# Copyright 2017 The Open GEE Contributors
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

set +x

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
    if [ -f /etc/init.d/gefusion ]; then
        service gefusion stop
    fi

    load_systemrc_config
    check_asset_root_volume_size

    # Check for invalid asset names:
    INVALID_ASSETROOT_NAMES=$(find "$ASSET_ROOT" -type d -name "*[\\\&\%\'\"\*\=\+\~\`\?\<\>\:\; ]*" 2> /dev/null)

    if [ -n "$INVALID_ASSETROOT_NAMES" ]; then
        show_invalid_assetroot_name "$INVALID_ASSETROOT_NAMES"
    fi

    create_users_and_groups
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_asset_root_volume_size()
{
    local VOLUME_PATH="$ASSET_ROOT"

    # Find the deepest existing directory in the path to $ASSET_ROOT:
    while [ ! -d "$VOLUME_PATH" ]; do
        VOLUME_PATH=$(dirname "$VOLUME_PATH")
    done

    ASSET_ROOT_VOLUME_SIZE=$(df -k "$VOLUME_PATH" | grep -v Avail | tr -s ' ' | cut -d ' ' -f 4)

    if [[ "$ASSET_ROOT_VOLUME_SIZE" -lt "$MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB" ]]; then
        MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB=$(expr "$MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB" / 1024 / 1024)

        cat <<END
The asset root volume [$ASSET_ROOT] has only $ASSET_ROOT_VOLUME_SIZE KB available.
We recommend that an asset root directory have a minimum of $MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB GB of free disk space.
END
    fi
}

create_users_and_groups()
{
    # Add user if it does not exist:
    if [ -z "$(getent passwd "$GEFUSIONUSER")" ]; then
		mkdir -p "$BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER"
		useradd --home "$BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER" \
            --system --gid "$GEGROUP" "$GEFUSIONUSER"
        keyvalue_file_set "$GEE_INSTALL_KV_PATH" gefusionuser_existed "false"
	else
		# The user already exists -- update primary group:
		usermod -g "$GEGROUP" "$GEFUSIONUSER"
        keyvalue_file_set "$GEE_INSTALL_KV_PATH" gefusionuser_existed "true"
    fi
}

show_invalid_assetroot_name()
{
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
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall $@
