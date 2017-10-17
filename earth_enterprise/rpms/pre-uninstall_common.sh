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
# from common.sh:
# directory locations
BININSTALLROOTDIR="/etc/init.d"

#------------------------------------------------------------------------------
# user names
GEAPACHEUSER_NAME=""
GEPGUSER_NAME=""
GEGROUP_NAME=""
GEAPACHEUSER_EXISTS=""
GEPGUSER_EXISTS=""
GEGROUP_EXISTS=""

main_preuninstall()
{
    get_user_names
    remove_users_groups
}

get_user_names()
{
  GEAPACHEUSER_NAME=`cat $BININSTALLROOTDIR/gevars.sh | grep GEAPACHEUSER | cut  -d'=' -f2`
  GEPGUSER_NAME=`cat $BININSTALLROOTDIR/gevars.sh | grep GEPGUSER | cut  -d'=' -f2`
  GEGROUP_NAME=`cat $BININSTALLROOTDIR/gevars.sh | grep GEGROUP | cut  -d'=' -f2`

  # Make sure the users and group exist
  GEAPACHEUSER_EXISTS=$(getent passwd $GEAPACHEUSER_NAME)
  GEPGUSER_EXISTS=$(getent passwd $GEPGUSER_NAME)
  GEGROUP_EXISTS=$(getent group $GEGROUP_NAME)
}

remove_users_groups()
{
    remove_account $GEAPACHEUSER_NAME "user" "$GEAPACHEUSER_EXISTS"
    remove_account $GEPGUSER_NAME "user" "$GEPGUSER_EXISTS"
    remove_account $GEGROUP_NAME "group" "$GEGROUP_EXISTS"
}

remove_account() {
    # arg $1: name of account to remove
    # arg $2: account type - "user" or "group"
    # arg $3: non-empty string if the user exists
    if [ ! -z "$3" ]; then
        echo -e "Deleting $2 $1"
        eval "$2del $1"
    fi
}

#-----------------------------------------------------------------
# Pre-Uninstall Main
#-----------------------------------------------------------------
main_preuninstall "$@"
