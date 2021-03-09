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

BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"
GEEF="$GEE Fusion"
LONG_VERSION="$(cat ../version.txt)"

ROOT_USERNAME="root"

MACHINE_OS=""
MACHINE_OS_VERSION=""
MACHINE_OS_FRIENDLY=""

# directory locations
BININSTALLROOTDIR="/etc/init.d"
BININSTALLPROFILEDIR="/etc/profile.d"
BASEINSTALLLOGROTATEDIR="/etc/logrotate.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"
TMPINSTALLDIR="/tmp/fusion_os_install"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
CHKCONFIG="/sbin/chkconfig"
KH_SYSTEMRC="/usr/keyhole/etc/systemrc"

# derived directories
BASEINSTALLGDALSHAREDIR="$BASEINSTALLDIR_OPT/share/gdal"
GENERAL_LOG="$BASEINSTALLDIR_VAR/log"
INSTALL_LOG_DIR="$BASEINSTALLDIR_OPT/install"
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"
FUSIONBININSTALL="$BININSTALLROOTDIR/gefusion"
SOURCECODEDIR=$(dirname $(dirname $(readlink -f "$0")))
TOPSOURCEDIR_EE=$(dirname $SOURCECODEDIR)
GESERVERBININSTALL="$BININSTALLROOTDIR/geserver"
GEHTTPD="$BASEINSTALLDIR_OPT/gehttpd"
GEHTTPD_CONF="$GEHTTPD/conf.d"

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"
HOSTNAME_S="$(hostname -s | $NEWLINECLEANER)"
HOSTNAME_A="$(hostname -a | $NEWLINECLEANER)"

NUM_CPUS="$(grep processor /proc/cpuinfo | wc -l | $NEWLINECLEANER)"

SUPPORTED_OS_LIST=("Ubuntu", "Red Hat Enterprise Linux (RHEL)", "CentOS", "Linux Mint")
UBUNTUKEY="ubuntu"
REDHATKEY="rhel"
CENTOSKEY="centos"
OS_RELEASE1="/etc/os-release"
OS_RELEASE2="/etc/system-release"

xml_file_get_xpath()
{
    local FILE="$1"
    local XPATH="$2"

    # Warning: `xmllint --noent` doesn't recognize named entities like
    # "&lt;" and "&gt;":
    echo "cat $XPATH" | xmllint --noent --nonet --shell "$FILE" | tail -2 | head -1
}

is_package_installed()
{
    # args: $1: Ubuntu package
    # args: $2: RHEL package

    if [ "$MACHINE_OS" == "$UBUNTUKEY" ] && [ ! -z "$1" ]; then
        if [[ ! -z "$(dpkg -l $1 | grep "^ii")" ]]; then
            return 0
        fi
    elif { [ "$MACHINE_OS" == "$REDHATKEY" ] || [ "$MACHINE_OS" == "$CENTOSKEY" ]; } && [ ! -z "$2" ]; then
        if [[ ! -z "$(rpm -qa | grep ^$2)" ]]; then
            return 0
        fi
    fi

    return 1
}

software_check()
{
    local software_check_retval=0

    # args: $1: name of script
    # args: $2: Ubuntu package
    # args: $3: RHEL package

    if ! is_package_installed $2 $3 ; then
        if [ "$MACHINE_OS" == "$UBUNTUKEY" ] && [ ! -z "$2" ]; then
            echo -e "\nInstall $2 and restart the $1."
            software_check_retval=1
        elif { [ "$MACHINE_OS" == "$REDHATKEY" ] || [ "$MACHINE_OS" == "$CENTOSKEY" ]; } && [ ! -z "$3" ]; then
            echo -e "\nInstall $3 and restart the $1."
            software_check_retval=1
        else
            echo -e "\nThe installer could not determine your machine's operating system."
            echo -e "Supported Operating Systems: ${SUPPORTED_OS_LIST[*]}\n"
            software_check_retval=1
        fi
    fi

    return $software_check_retval
}

determine_os()
{
    local retval=0
    local test_os=""
    local test_versionid=""

    if [ -f "$OS_RELEASE1" ] || [ -f "$OS_RELEASE2" ]; then
        if [ -f "$OS_RELEASE1" ]; then
            test_os="$(cat $OS_RELEASE1 | sed -e 's:\"::g' | grep ^NAME= | sed 's:name=::gI')"
            test_versionid="$(cat $OS_RELEASE1 | sed -e 's:\"::g' | grep ^VERSION_ID= | sed 's:version_id=::gI')"
        else
            test_os="$(cat $OS_RELEASE2 | sed 's:[0-9\.] *::g')"
            test_versionid="$(cat $OS_RELEASE2 | sed 's:[^0-9\.]*::g')"
        fi

        MACHINE_OS_FRIENDLY="$test_os $test_versionid"
        MACHINE_OS_VERSION=$test_versionid

        if [[ "${test_os,,}" == "ubuntu"* ]]; then
            MACHINE_OS=$UBUNTUKEY
        elif [[ "${test_os,,}" == "linux mint"* ]]; then
            MACHINE_OS=$UBUNTUKEY
        elif [[ "${test_os,,}" == "red hat"* ]]; then
            MACHINE_OS=$REDHATKEY
        elif [[ "${test_os,,}" == "centos"* ]]; then
            MACHINE_OS=$CENTOSKEY
        else
            MACHINE_OS=""
            echo -e "\nThe installer could not determine your machine's operating system."
            echo -e "Supported Operating Systems: ${SUPPORTED_OS_LIST[*]}\n"
            retval=1
        fi
    else
        echo -e "\nThe installer could not determine your machine's operating system."
        echo -e "Missing file: $OS_RELEASE1 or $OS_RELEASE2\n"
        retval=1
    fi

    return $retval
}


show_opengee_package_installed()
{
    #args: $1 "install" or "uninstall"
    #args: $2 Software name, "Open GEE Fusion", "Open GEE Server"

    echo -e "\nOpen GEE packages installed."
    echo -e "Cannot $1 $2 because Open GEE packages have been installed with a package manager."
}


run_as_user() {
    local use_su=`su $1 -c 'echo -n 1' 2> /dev/null  || echo -n 0`
    if [ "$use_su" -eq 1 ] ; then
        >&2 echo "cd / ;su $1 -c \"$2\""
        ( cd / ;su $1 -c "$2" )
    else
        >&2 echo "cd / ;sudo -u $1 $2"
        ( cd / ;sudo -u $1 $2 )
    fi
}

show_no_tmp_dir_message()
{
    echo -e "\nThe temp install directory specified [$1] does not exist."
    echo -e "Please specify the path of the extracted install files or first run"
    echo -e "python2.7 /usr/bin/scons [-j8] release=1 [installdir=$1] stage_install\n"
}

show_corrupt_tmp_dir_message()
{
    echo -e "\nThe temp install directory specified [$1] is corrupt (failed on copy)."
    echo -e "Please see $2 for details"
    echo -e "Please specify the path of the extracted install files or first run"
    echo -e "python2.7 /usr/bin/scons [-j8] release=1 [installdir=$1] stage_install\n"
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
    # if the user already existed but the directory had been removed at some point
    if [ ! -d "$BASEINSTALLDIR_OPT/.users/$1" ]; then
        mkdir -p $BASEINSTALLDIR_OPT/.users/$1
    fi
    chown -R $1:$GROUPNAME $BASEINSTALLDIR_OPT/.users/$1
}

create_links()
{
    printf "Setting up system links..."

    if [ ! -L "$BASEINSTALLDIR_OPT/etc" ]; then
        ln -s $BASEINSTALLDIR_ETC $BASEINSTALLDIR_OPT/etc
    fi

    if [ ! -L "$BASEINSTALLDIR_OPT/log" ]; then
        ln -s $BASEINSTALLDIR_VAR/log $BASEINSTALLDIR_OPT/log
    fi

    if [ ! -L "$BASEINSTALLDIR_OPT/run" ]; then
        ln -s $BASEINSTALLDIR_VAR/run $BASEINSTALLDIR_OPT/run
    fi

    if [ ! -L "$BASEINSTALLDIR_OPT/gehttpd/htdocs/shared_assets/docs" ]; then
        sudo ln -s $BASEINSTALLDIR_OPT/share/doc $BASEINSTALLDIR_OPT/gehttpd/htdocs/shared_assets/docs
    fi

    printf "DONE\n"
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
  echo "wsgi service: $wsgi_running_str"

  return $retval
}

backup_server()
{
  export BACKUP_DIR=$BACKUP_DIR

  if [ ! -d $BACKUP_DIR ]; then
    mkdir -p $BACKUP_DIR
  fi

  # Copy gehttpd directory.
  # Do not back up folder /opt/google/gehttpd/htdocs/cutter/globes.
  # Do not back up folders /opt/google/gehttpd/{bin, lib, manual, modules}
  rsync -rltpu $BASEINSTALLDIR_OPT/gehttpd $BACKUP_DIR --exclude bin --exclude lib --exclude manual --exclude modules --exclude htdocs/cutter/globes

  # Copy other files.
  cp -f /etc/init.d/gevars.sh $BACKUP_DIR
  cp -rf /etc/opt/google/openldap $BACKUP_DIR

  # Change the ownership of the backup folder.
  chown -R root:root $BACKUP_DIR

  echo -e "\nThe existing $GEES $LONG_VERSION configuration files have been backed up to the following location:"
  echo -e $BACKUP_DIR
  echo -e "The source volume(s) and asset root remain unchanged."
}

get_array_index()
{
    # need to have a set test value -- technically, the return status is an unsigned 8 bit value, so negative numbers won't work
    # need a value large enough that can be tested against
    local get_array_index_retval=$INVALID_INDEX

    # args $1: array
    # args $2: choice/selection

    local array_list=("${!1}")
    local selection=$2

    for i in "${!array_list[@]}";
    do
        if [[ "${array_list[$i]}" == "${selection}" ]]; then
            get_array_index_retval=$i
            break
        fi
    done

    return $get_array_index_retval
}

prompt_to_action()
{
    # args- $1: array
    # args- $2: repeatable prompt

    local prompt_to_action_choice=""
    local prompt_to_action_validAnswers=("${!1}")

    while [[ " ${prompt_to_action_validAnswers[*]} " != *"${prompt_to_action_choice^^} "* ]] || [ -z "$prompt_to_action_choice" ]
    do
        printf "$2 "
        read -r prompt_to_action_choice
    done

    get_array_index prompt_to_action_validAnswers[@] ${prompt_to_action_choice^^}
    prompt_to_action_retval=$?

    return $prompt_to_action_retval
}

prompt_to_quit()
{
    # args- $1: repeatable prompt
    local prompt_to_quit_retval=0
    local prompt_to_quit_validAnswers=(X C)
    local prompt_to_quit_index=1

    prompt_to_action prompt_to_quit_validAnswers[@] "$1"
    prompt_to_quit_index=$?

    if [ $prompt_to_quit_index -eq 1 ]; then
        prompt_to_quit_retval=0
    else
        prompt_to_quit_retval=1
    fi

    return $prompt_to_quit_retval
}

# Returns the default user
get_default_user()
{
	# $1 -- is file/directory from which to obtain user
	# $2 -- user to return if file/directory does not exist (or unable to stat file)
	local USER=$(stat --printf="%U" $1 2> /dev/null)
	[ -z "$USER" ] && USER=$2
	echo "$USER"
}

# Returns the default group
get_default_group()
{
	# $1 -- is file/directory from which to obtain group
	# $2 -- group to return if file/directory does not exist (or unable to stat file)
	local GRP=$(stat --printf="%G" $1 2> /dev/null)
	[ -z "$GRP" ] && GRP=$2
	echo "$GRP"
}

# For given directory, array containing directory, size, available, and mount point
get_volume_info()
{
	local info=()
	local DIR=$1

	# Check if dir exists.  If so, do a df at this point
	while true; do
		if [ -d "$DIR" ] ; then
			info=($1 $(df -k $DIR --output=size,avail,target | tail -1))
			break
		fi

		# Drop last part of path and retry
		DIR=${DIR%/*}
		# Check if reached the root
		if [ -z $DIR ]; then
			info=($1 $(df -k / --output=size,avail,target | tail -1))
			break
		fi
	done
	echo "${info[@]}"
}
