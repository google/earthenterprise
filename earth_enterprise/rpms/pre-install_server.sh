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

#------------------------------------------------------------------------------
# from common.sh:

BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"
LONG_VERSION="5.2.1"

ROOT_USERNAME="root"

MACHINE_OS=""
MACHINE_OS_VERSION=""
MACHINE_OS_FRIENDLY=""

# directory locations
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"

# derived directories
BASEINSTALLGDALSHAREDIR="$BASEINSTALLDIR_OPT/share/gdal"
GENERAL_LOG="$BASEINSTALLDIR_VAR/log"
INSTALL_LOG_DIR="$BASEINSTALLDIR_OPT/install"
GEHTTPD="$BASEINSTALLDIR_OPT/gehttpd"
GEHTTPD_CONF="$GEHTTPD/conf.d"

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"

SUPPORTED_OS_LIST=("Ubuntu", "Red Hat Enterprise Linux (RHEL)", "CentOS", "Linux Mint")
UBUNTUKEY="ubuntu"
REDHATKEY="rhel"
CENTOSKEY="centos"

software_check()
{
    local software_check_retval=0

    # args: $1: name of script
    # args: $2: Ubuntu package
    # args: $3: RHEL package

    if [ "$MACHINE_OS" == "$UBUNTUKEY" ] && [ ! -z "$2" ]; then
        if [[ -z "$(dpkg --get-selections | sed s:install:: | sed -e 's:\s::g' | grep ^$2\$)" ]]; then
            echo -e "\nInstall $2 and restart the $1."
            software_check_retval=1
        fi
    elif { [ "$MACHINE_OS" == "$REDHATKEY" ] || [ "$MACHINE_OS" == "$CENTOSKEY" ]; } && [ ! -z "$3" ]; then
        if [[ -z "$(rpm -qa | grep ^$3\$)" ]]; then
            echo -e "\nInstall $3 and restart the $1."
            software_check_retval=1
        fi
    else 
        echo -e "\nThe $1 could not determine your machine's operating system."
        echo -e "Supported Operating Systems: ${SUPPORTED_OS_LIST[*]}\n"
        software_check_retval=1
    fi

    return $software_check_retval
}

check_bad_hostname() {
    if [ -z "$HOSTNAME" ] || [[ " ${BADHOSTNAMELIST[*]} " == *"${HOSTNAME,,} "* ]]; then
        show_badhostname

        if [ $BADHOSTNAMEOVERRIDE == true ]; then
            echo -e "Continuing the installation process...\n"
            # 0 = true (in bash)
            return 0
        else
            echo -e "Exiting the installer.  If you wish to continue, re-run this command with the -hnf 'Hostname Override' flag.\n"
            # 1 = false
            return 1
        fi
    fi
}

show_badhostname()
{
    echo -e "\nYour server [$HOSTNAME] contains an invalid hostname value which typically"
    echo -e "indicates an automatically generated hostname that might change over time."
    echo -e "A subsequent hostname change would cause configuration issues for the "
    echo -e "$SOFTWARE_NAME software.  Invalid values: ${BADHOSTNAMELIST[*]}."
}

check_mismatched_hostname() {
    if [ $HOSTNAME != $HOSTNAME_F ]; then
        show_mismatchedhostname

        if [ $MISMATCHHOSTNAMEOVERRIDE == true ]; then
            echo -e "Continuing the installation process...\n"
            # 0 = true (in bash)
            return 0
        else
            echo -e "Exiting the installer.  If you wish to continue, re-run this command with the -hnmf 'Hostname Mismatch Override' flag.\n"
            # 1 = false
            return 1
        fi
    fi
}

show_mismatchedhostname()
{
    echo -e "\nThe hostname of this machine does not match the fully-qualified hostname."
    echo -e "$SOFTWARE_NAME requires that they match for local publishing to function properly."
    # Chris: I took out the message about updating the hostname because this script doesn't actually do it.
    # TODO Add some instructions on how to update hostname
}

check_group() {
    GROUP_EXISTS=$(getent group $GROUPNAME)

    # add group if it does not exist
    if [ -z "$GROUP_EXISTS" ]; then
        groupadd -r $GROUPNAME &> /dev/null 
        NEW_GEGROUP=true 
    fi
}

check_username() {
    USERNAME_EXISTS=$(getent passwd $1)

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p $BASEINSTALLDIR_OPT/.users/$1
        useradd --home $BASEINSTALLDIR_OPT/.users/$1 --system --gid $GROUPNAME $1
        NEW_GEFUSIONUSER=true
    else
        echo "User $1 exists"
        # user already exists -- update primary group
        usermod -g $GROUPNAME $1
    fi
}

check_server_processes_running()
{
  printf "\nChecking geserver services:\n"
  local retval=1

  # i) Check if postgres is running
  local post_master_running=$( ps -ef | grep postgres | grep -v grep )
  local post_master_running_str="false"

  # ii) Check if gehttpd is running
  local gehttpd_running=$( ps -ef | grep gehttpd | grep -v grep )
  local gehttpd_running_str="false"

  # iii) Check if wsgi is running
  local wsgi_running=$( ps -ef | grep wsgi:ge_ | grep -v grep )
  local wsgi_running_str="false"

  if [ -n "$post_master_running" ]; then
    retval=0
    post_master_running_str="true"
  fi
  echo "postgres service: $post_master_running_str"

  if [ -n "$gehttpd_running" ]; then
    retval=0
    gehttpd_running_str="true"
  fi
  echo "gehttpd service: $gehttpd_running_str"

  if [ -n "$wsgi_running" ]; then
    retval=0
    wsgi_running_str="true"
  fi
  echo "wsgi service: $gehttpd_running_str"

  return $retval
}

#------------------------------------------------------------------------------

# config values
PUBLISHER_ROOT="/gevol/published_dbs"
DEFAULTGROUPNAME="gegroup"
GROUPNAME=$DEFAULTGROUPNAME
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME

# script arguments
BACKUPSERVER=true
BADHOSTNAMEOVERRIDE=false
MISMATCHHOSTNAMEOVERRIDE=false

# user names
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"


#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
  # 5a) Check previous installation
  if [ -f "$SERVERBININSTALL" ]; then
    IS_NEWINSTALL=false
  else
    IS_NEWINSTALL=true
  fi

  # 7) Check prerequisite software
  check_prereq_software

  # check to see if GE Server processes are running
  if check_server_processes_running; then
    #STOP PROCESSES
  fi

  # 6) Check valid host properties
  if ! check_bad_hostname; then
  fi

  if ! check_mismatched_hostname; then
  fi

  # 8) Check if group and users exist
  check_group

  check_username $GEAPACHEUSER_NAME
  check_username $GEPGUSER_NAME

  # 10) Configure Publish Root
  configure_publish_root
}

check_prereq_software()
{
  local check_prereq_software_retval=0
  local script_name="$GEES $LONG_VERSION installer"

  if ! software_check "$script_name" "libxml2-utils" "libxml2.*x86_64"; then
    check_prereq_software_retval=1
  fi

  if ! software_check "$script_name" "python2.[67]" "python-2.[67].*"; then
    check_prereq_software_retval=1
  fi

  return $check_prereq_software_retval
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

configure_publish_root()
{
  # Update PUBLISHER_ROOT if geserver already installed
  local STREAM_SPACE="$GEHTTPD_CONF/stream_space"
  if [ -e $STREAM_SPACE ]; then
    PUBLISHER_ROOT=`cat $STREAM_SPACE |cut -d" " -f3 |sed 's/.\{13\}$//'`
  fi
}

#-----------------------------------------------------------------
# Install Functions
#-----------------------------------------------------------------
#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------

#-----------------------------------------------------------------
main_preinstall "$@"
