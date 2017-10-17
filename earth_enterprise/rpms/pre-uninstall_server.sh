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
# versions and user names
GEE="Google Earth Enterprise"

# directory locations
BININSTALLROOTDIR="/etc/init.d"

#------------------------------------------------------------------------------
# script arguments
DELETE_GROUP=true

# user names
GEAPACHEUSER_NAME=""
GEPGUSER_NAME=""
GEGROUP_NAME=""
GEAPACHEUSER_EXISTS=""
GEPGUSER_EXISTS=""
GEGROUP_EXISTS=""

PUBLISH_ROOT_CONFIG_PATH="$BASEINSTALLDIR_OPT/gehttpd/conf.d"

# additional variables
HAS_FUSION=false
SEARCH_SPACE_PATH=""
STREAM_SPACE_PATH=""

main_preuninstall()
{
    service geserver stop

    # Determine if fusion is installed
    if [ -f "$BININSTALLROOTDIR/gefusion" ]; then
        HAS_FUSION=true
    else
        HAS_FUSION=false
    fi

    # get the GE user names
    get_user_names

    # check if the group can be safely deleted
    check_group_delete

    # find the publish root
    get_publish_roots
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

check_group_delete()
{
    if [ $DELETE_GROUP == true ] && [ $HAS_FUSION == true ]; then
        echo -e "\nNote: the GEE group [$GEGROUP_NAME] will not be deleted because $GEEF is installed on"
        echo -e "this server. $GEEF uses this account too."
        echo -e "The group account will be deleted when $GEEF is uninstalled."
        DELETE_GROUP=false
    fi
}

get_publish_roots()
{
    STREAM_SPACE_PATH=$(get_publish_path "stream_space")
    SEARCH_SPACE_PATH=$(get_publish_path "search_space")
}

get_publish_path()
{
    local config_file="$PUBLISH_ROOT_CONFIG_PATH/$1"
    local publish_path=`grep $1 "$config_file" | cut -d ' ' -f 3`
    echo $publish_path
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
