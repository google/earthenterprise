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
# Definitions
GEE="Google Earth Enterprise"
#-----------------------------------------------------------------

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
    if test -f /etc/init.d/gefusion ; then
        service gefusion stop ; fi

    check_username $GEFUSIONUSER gefusionuser_existed
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_username() {
    USERNAME_EXISTS=$(getent passwd $1)
    KEYVALUE_EXISTS="$2"

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p "$BASEINSTALLDIR_OPT/.users/$1"
        useradd --home "$BASEINSTALLDIR_OPT/.users/$1" --system --gid "$GEGROUP" "$1"
        if [ ! -z "$KEYVALUE_EXISTS" ] ; then
            keyvalue_file_set "$GEE_INSTALL_KV_PATH" "$KEYVALUE_EXISTS" "false" ; fi
    else
        # user already exists -- update primary group
        usermod -g "$GEGROUP" "$1"
        if [ ! -z "$KEYVALUE_EXISTS" ] ; then
            keyvalue_file_set "$GEE_INSTALL_KV_PATH" "$KEYVALUE_EXISTS" "true" ; fi
    fi
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall "$@"
