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

# versions and user names
GEES="Google Earth Enterprise Server"
GEE="Google Earth Enterprise"
LONG_VERSION="5.1.3"
SHORT_VERSION="5.1"

# script arguments
BACKUPSERVER=true

# user names
ROOT_USERNAME="root"
GEAPACHEUSER_NAME=""
GEPGUSER_NAME=""
GEFUSIONUSER_NAME=""
GEGROUP_NAME=""

# base directories
BININSTALLROOTDIR="/etc/init.d"
BININSTALLPROFILEDIR="/etc/profile.d"
BASEINSTALLLOGROTATEDIR="/etc/logrotate.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"

# derived directories
BACKUP_DIR="$BASEINSTALLDIR_VAR/server-backups/$(date +%Y_%m_%d.%H%M%S)"

# additional variables
HAS_FUSION=false

main_preuninstall()
{
  show_intro

  # Root/Sudo check
  if [ "$EUID" != "0" ]; then
    show_need_root
    exit 1
  fi

  if ! determine_os; then
    exit 1
  fi

  # check to see if GE Server processes are running
  if check_server_processes_running; then
    show_server_running_message
    exit 1
  fi

  # get the GE user names
  get_user_names

  # Perform backup
  if [ $BACKUPSERVER == true ]; then
    # TODO: verify that the backup succeeded
    backup_server
  fi

  # Determine if fusion is installed
  if [ -f "$BININSTALLROOTDIR/gefusion" ]; then
    HAS_FUSION=true
  else
    HAS_FUSION=false
  fi

  # TODO: delete users and groups
}

main_uninstall()
{
  remove_server_daemon
  remove_files_from_target
  show_final_success_message
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
  GEFUSIONUSER_NAME=`cat $BININSTALLROOTDIR/gevars.sh | grep GEFUSIONUSER | cut  -d'=' -f2`
  GEGROUP_NAME=`cat $BININSTALLROOTDIR/gevars.sh | grep GEGROUP | cut  -d'=' -f2`
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
  fi

  rm -rf $BASEINSTALLDIR_OPT/gehttpd/htdocs/shared_assets/apache_logs.html
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

  rm -f $BININSTALLROOTDIR/gevars.sh

  printf "DONE\n"
}

show_final_success_message()
{
    echo -e "\n-------------------"
    echo -e "\n$GEES $LONG_VERSION was successfully uninstalled."
    echo -e "The backup configuration files are located in:"
    echo -e "\n$BACKUP_DIR\n"
}

#-----------------------------------------------------------------
# Pre-Uninstall Main
#-----------------------------------------------------------------
main_preuninstall

#-----------------------------------------------------------------
# Uninstall Main
#-----------------------------------------------------------------
main_uninstall

