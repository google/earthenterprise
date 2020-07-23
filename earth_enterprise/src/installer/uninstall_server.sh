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

# get script directory
SCRIPTDIR=`dirname $0`

. $SCRIPTDIR/common.sh

# script arguments
BACKUPSERVER=true
DELETE_USERS=true
DELETE_GROUP=true

# user names
GEAPACHEUSER_NAME=""
GEPGUSER_NAME=""
GEGROUP_NAME=""
GEAPACHEUSER_EXISTS=""
GEPGUSER_EXISTS=""
GEGROUP_EXISTS=""

BACKUP_DIR="$BASEINSTALLDIR_VAR/server-backups/$(date +%Y_%m_%d.%H%M%S)"
PUBLISH_ROOT_CONFIG_PATH="$BASEINSTALLDIR_OPT/gehttpd/conf.d"

# additional variables
HAS_FUSION=false
SEARCH_SPACE_PATH=""
STREAM_SPACE_PATH=""

main_preuninstall()
{
  # parse command line arguments
  if ! parse_arguments "$@"; then
    exit 1
  fi

  show_intro

  # Root/Sudo check
  if [ "$EUID" != "0" ]; then
    show_need_root
    exit 1
  fi

  if ! determine_os; then
    exit 1
  fi

  if is_package_installed "opengee-common" "opengee-common"; then
    show_opengee_package_installed "uninstall" "$GEES"
    exit 1
  fi

  # check to see if GE Server processes are running
  if check_server_processes_running; then
    show_server_running_message
    exit 1
  fi

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

  if ! prompt_uninstall_confirmation; then
    exit 1
  fi

  # Perform backup
  if [ $BACKUPSERVER == true ]; then
    backup_server
  fi
}

main_uninstall()
{
  remove_server_daemon
  remove_files_from_target
  change_publish_root_ownership
  remove_users_groups
  if [ `command -v systemctl` ]; then systemctl daemon-reexec; fi
  show_final_success_message
}

show_help()
{
  echo -e "\nUsage:  sudo ./uninstall_server.sh [-h -ndgu -nobk]\n"
  echo -e "-h \t\tHelp - display this help screen"
  echo -e "-ndgu \t\tDo Not Delete GEE Users and Group - do not delete the GEE user accounts and group.  Default is to delete both."
  echo -e "-nobk \t\tNo Backup - do not backup the current GEE Server setup. Default is to backup the setup before uninstalling.\n"
}

parse_arguments()
{
  local retval=0

  while [ $# -gt 0 ]; do
    case "${1,,}" in
      -h|-help|--help)
        show_help
        retval=1
        break
        ;;
      -nobk)
        BACKUPSERVER=false
        ;;
      -ndgu)
        DELETE_USERS=false
        DELETE_GROUP=false
        ;;
      *)
        echo -e "\nArgument Error: $1 is not a valid argument."
        show_help
        retval=1
        break
        ;;
    esac

    if [ $# -gt 0 ]; then
      shift
    fi
  done

  return $retval;
}

show_intro()
{
  echo -e "\nUninstalling $GEES $LONG_VERSION"
  echo -e "\nThis will remove features installed by the GEE Server installer."
  echo -e "It will NOT remove files and folders created after the installation."
}

show_need_root()
{
  echo -e "\nYou must have root privileges to uninstall $GEES.\n"
  echo -e "Log in as the $ROOT_USERNAME user or use the 'sudo' command to run this installer."
  echo -e "The installer must exit."
}

show_server_running_message()
{
  echo -e "\n$GEES has active running processes."
  echo -e "You must stop all geserver services before running this installer."
  echo -e "Please stop the server using the command \"$BININSTALLROOTDIR/geserver stop\".\n"
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

prompt_uninstall_confirmation()
{
  local backupStringValue=""
  local deleteUserValue=""
  local deleteGroupValue=""

  if [ $BACKUPSERVER == true ]; then
    backupStringValue="YES"
  else
    backupStringValue="NO"
  fi

  if [ $DELETE_USERS == true ]; then
    deleteUserValue="YES - Delete GEE users [$GEAPACHEUSER_NAME, $GEPGUSER_NAME]"
  else
    deleteUserValue="NO"
  fi

  if [ $DELETE_GROUP == true ]; then
    deleteGroupValue="YES - Delete GEE group [$GEGROUP_NAME]"
  else
    deleteGroupValue="NO"
  fi

  echo -e "\nYou have chosen to uninstall $GEES with the following settings:\n"
  echo -e "Backup Server: \t$backupStringValue"
  echo -e "Delete Users: \t$deleteUserValue"
  echo -e "Delete Group: \t$deleteGroupValue\n"

  if [ $DELETE_USERS == true ] || [ $DELETE_GROUP == true ]; then
    echo -e "You have chosen to remove the GEE users and/or group."
    echo -e "Note: it may take some time to change ownership of the publish roots to \"$ROOT_USERNAME\".\n"
  fi

  if ! prompt_to_quit "X (Exit) the uninstaller and change the above settings - C (Continue) to uninstall."; then
    echo -e "Exiting the uninstaller.\n"
    return 1
  else
    echo -e "\nProceeding with uninstallation..."
    return 0
  fi
}

remove_server_daemon()
{
  test -f /sbin/chkconfig && /sbin/chkconfig --del geserver
  test -f /usr/sbin/update-rc.d && /usr/sbin/update-rc.d -f geserver remove
  rm $BININSTALLROOTDIR/geserver 
}

remove_files_from_target()
{
  printf "\nRemoving files from target directories..."

  if [ $HAS_FUSION == false ]; then
    # don't delete the install log directory (which is where the uninstall log is kept)
    # rm -rf $BASEINSTALLDIR_OPT/install

    rm -rf $BASEINSTALLDIR_ETC
    rm -rf $BASEINSTALLDIR_VAR/log
    rm -rf $BASEINSTALLDIR_VAR/run
    rm -rf $BASEINSTALLDIR_OPT/log
    rm -rf $BASEINSTALLDIR_OPT/run
    rm -rf $BASEINSTALLDIR_OPT/etc
    rm -rf $BASEINSTALLDIR_OPT/.users

    rm -rf $BASEINSTALLDIR_OPT/share
    rm -rf $BASEINSTALLDIR_OPT/bin
    rm -rf $BASEINSTALLDIR_OPT/qt
    rm -rf $BASEINSTALLDIR_OPT/lib
    rm -rf $BASEINSTALLDIR_OPT/gepython
    rm -rf $BASEINSTALLDIR_VAR/openssl

    rm -f $BININSTALLROOTDIR/gevars.sh
  fi

  rm -rf $BASEINSTALLDIR_OPT/geecheck/geecheck.conf
  if [ -d "$BASEINSTALLDIR_OPT/geecheck" ]; then
    rmdir --ignore-fail-on-non-empty $BASEINSTALLDIR_OPT/geecheck  # Only remove this directory if it's empty
  fi

  rm -rf $BASEINSTALLDIR_OPT/search
  rm -rf $BASEINSTALLDIR_VAR/pgsql
  rm -rf $BASEINSTALLDIR_OPT/gehttpd

  rm -f $BININSTALLPROFILEDIR/ge-server.sh
  rm -f $BININSTALLPROFILEDIR/ge-server.csh

  rm -rf $BASEINSTALLLOGROTATEDIR/gehttpd

  printf "DONE\n"
}

change_publish_root_ownership()
{
  if [ $DELETE_USERS == true ] || [ $DELETE_GROUP == true ]; then
    for publish_root in "$STREAM_SPACE_PATH" "$SEARCH_SPACE_PATH"; do
      if [ -d "$publish_root" ]; then
        echo -e "\nChanging ownership of $publish_root to $ROOT_USERNAME:$ROOT_USERNAME."
        chown -R $ROOT_USERNAME:$ROOT_USERNAME "$publish_root"
      else
        echo -e "\nPublish root $publish_root does not exist.  Continuing."
      fi
    done
  fi
}

remove_users_groups()
{
  if [ $DELETE_USERS == true ]; then
    remove_account $GEAPACHEUSER_NAME "user" "$GEAPACHEUSER_EXISTS"
    remove_account $GEPGUSER_NAME "user" "$GEPGUSER_EXISTS"
  fi
  if [ $DELETE_GROUP == true ]; then
    remove_account $GEGROUP_NAME "group" "$GEGROUP_EXISTS"
  fi
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

show_final_success_message()
{
  echo -e "\n-------------------"
  echo -e "\n$GEES $LONG_VERSION was successfully uninstalled."
  if [ $BACKUPSERVER == true ]; then
    echo -e "The backup configuration files are located in:"
    echo -e "\n$BACKUP_DIR"
  fi
  echo
}

#-----------------------------------------------------------------
# Pre-Uninstall Main
#-----------------------------------------------------------------
main_preuninstall "$@"

#-----------------------------------------------------------------
# Uninstall Main
#-----------------------------------------------------------------
main_uninstall

