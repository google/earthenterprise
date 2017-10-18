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

set +x

#------------------------------------------------------------------------------
# Directory locations:
BININSTALLROOTDIR="/etc/init.d"

#------------------------------------------------------------------------------
# Group names:
GEGROUP_NAME=""
GEGROUP_EXISTS=""

main_postuninstall()
{
    get_user_group_names
    remove_users_groups
}

get_user_group_names()
{
    GEGROUP_NAME=$(cat "$BININSTALLROOTDIR/gevars.sh" | grep GEGROUP | cut  -d'=' -f2)

    # Check if the group exists:
    GEGROUP_EXISTS=$(getent group "$GEGROUP_NAME")
}

remove_users_groups()
{
    [ -n "$GEGROUP_EXISTS" ] && remove_account "$GEGROUP_NAME" "group"
}

remove_account() {
    # arg $1: name of account to remove
    # arg $2: account type - "user" or "group"
    echo "Deleting $2 $1"
    "${2}del" "$1"
}

#-----------------------------------------------------------------
# Main Function:
#-----------------------------------------------------------------
main_postuninstall "$@"
#-----------------------------------------------------------------
