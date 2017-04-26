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

# config values
ASSET_ROOT="/gevol/assets"
SOURCE_VOLUME="/gevol/src"
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
DEFAULTGROUPNAME="gegroup"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME
GROUPNAME=$DEFAULTGROUPNAME

# directory locations
BININSTALLROOTDIR="/etc/init.d"
BININSTALLPROFILEDIR="/etc/profile.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"
TMPINSTALLDIR="/tmp/fusion_os_install"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
CHKCONFIG="/sbin/chkconfig"
OS_RELEASE1="/etc/os-release"
OS_RELEASE2="/etc/system-release"

# script arguments
BADHOSTNAMELIST=(empty linux localhost dhcp bootp)
BACKUPFUSION=true
BADHOSTNAMEOVERRIDE=false
MISMATCHHOSTNAMEOVERRIDE=false
START_FUSION_DAEMON=true
PURGE_TMP_DIR=true

# derived directories
BASEINSTALLGDALSHAREDIR="$BASEINSTALLDIR_OPT/share/gdal"
GENERAL_LOG="$BASEINSTALLDIR_VAR/log"
INSTALL_LOG_DIR="$BASEINSTALLDIR_OPT/install"
INSTALL_LOG="$INSTALL_LOG_DIR/fusion_install_$(date +%Y_%m_%d.%H%M%S).log"
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"
BACKUP_DIR="$BASEINSTALLDIR_VAR/fusion-backups/$(date +%Y_%m_%d.%H%sM%S)"
FUSIONBININSTALL="$BININSTALLROOTDIR/gefusion"
SOURCECODEDIR=$(dirname $(dirname $(readlink -f "$0")))
TOPSOURCEDIR_EE=$(dirname $SOURCECODEDIR)

# additional variables
GEEF="Google Earth Enterprise Fusion"
LONG_VERSION="5.1.3"
IS_NEWINSTALL=false
NEWLINECLEANER="sed -e s:\\n::g"
PUBLISH_ROOT_VOLUME=""
IS_64BIT_OS=false
ROOT_USERNAME="root"
SUPPORTED_OS_LIST=("Ubuntu", "Red Hat Enterprise Linux (RHEL)")
UBUNTUKEY="ubuntu"
REDHATKEY="rhel"
MACHINE_OS=""
MACHINE_OS_VERSION=""
MACHINE_OS_FRIENDLY=""

HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"
HOSTNAME_S="$(hostname -s | $NEWLINECLEANER)"
HOSTNAME_A="$(hostname -a | $NEWLINECLEANER)"
NUM_CPUS="$(grep processor /proc/cpuinfo | wc -l | $NEWLINECLEANER)"

SOURCE_VOLUME_PREEXISTING=false
ASSET_ROOT_PREEXISTING=false
ASSET_ROOT_VOLUME_SIZE=0
MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB=1048576
EXISTING_HOST=""
IS_SLAVE=false
NEW_GEFUSIONUSER=false
NEW_GEGROUP=false
INVALID_INDEX=255

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
	show_intro

	# Root/Sudo check
	if [ "$EUID" != "0" ]; then 
		show_need_root
		exit 1
	fi

	if [ -f "$FUSIONBININSTALL" ]; then
		IS_NEWINSTALL=false
	else
		IS_NEWINSTALL=true
	fi

	# Argument check
	if ! parse_arguments "$@"; then		
		exit 1
	fi

	if ! determine_os; then
        exit 1
    fi

	if ! check_prereq_software; then
		exit 1
	fi
	
	# check to see if GE Fusion processes are running
	if ! check_fusion_processes_running; then
		show_fusion_running_message
		exit 1
	fi

	# tmp dir check
	if [ ! -d "$TMPINSTALLDIR" ]; then
		show_no_tmp_dir_message $TMPINSTALLDIR
		exit 1
	fi

	load_systemrc_config

	# check for invalid asset names
	INVALID_ASSETROOT_NAMES=$(find $ASSET_ROOT -type d -name "*[\\\&\%\'\"\*\=\+\~\`\?\<\>\:\; ]*" 2> /dev/null)

	if [ ! -z "$INVALID_ASSETROOT_NAMES" ]; then
		show_invalid_assetroot_name $INVALID_ASSETROOT_NAMES
		exit 1
	fi
    
	if [ -z "$HOSTNAME" ] || [[ " ${BADHOSTNAMELIST[*]} " == *"${HOSTNAME,,} "* ]]; then
		show_badhostname

		if [ $BADHOSTNAMEOVERRIDE == true ]; then
			echo -e "Continuing the installation process...\n"
		else
			echo -e "Exiting the installer.  If you wish to continue, re-run this command with the -hnf 'Hostname Override' flag.\n"
			exit 1
		fi
	fi

	if [ $HOSTNAME != $HOSTNAME_F ]; then
		show_mismatchedhostname

		if [ $MISMATCHHOSTNAMEOVERRIDE == true ]; then
			echo -e "Continuing the installation process...\n"
		else
			echo -e "Exiting the installer.  If you wish to continue, re-run this command with the -hnmf 'Hostname Mismatch Override' flag.\n"
			exit 1
		fi		
	fi

	# 64 bit check
	if [[ "$(uname -i)" == "x86_64" ]]; then 
        IS_64BIT_OS=true 
	else
		echo -e "\n$GEEF $LONG_VERSION can only be installed on a 64 bit operating system."
		exit 1
    fi

	if ! prompt_install_confirmation; then
		exit 1
	fi

	if [ $BACKUPFUSION == true ]; then
		# Backing up current Fusion Install...
		backup_fusion
	fi
}

main_install()
{
	GROUP_EXISTS=$(getent group $GROUPNAME)
	USERNAME_EXISTS=$(getent passwd $GEFUSIONUSER_NAME)

	# add group if it does not exist
	if [ -z "$GROUP_EXISTS" ]; then
		groupadd -r $GROUPNAME &> /dev/null 
        NEW_GEGROUP=true 
	fi

	# add user if it does not exist
	if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p $BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER_NAME
		useradd -d $BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER_NAME -g gegroup $GEFUSIONUSER_NAME
        NEW_GEFUSIONUSER=true
    else
        # user already exists -- update primary group
        usermod -g $GROUPNAME $GEFUSIONUSER_NAME
	fi

	copy_files_to_target
	create_links
}

main_postinstall()
{
	create_system_main_directories

    if ! compare_asset_root_publishvolume; then
        exit 1
    fi

    if ! check_asset_root_volume_size; then
        exit 1
    fi

    setup_fusion_daemon
	
	# store fusion version
	echo $LONG_VERSION > $BASEINSTALLDIR_ETC/fusion_version

	chmod 755 $BININSTALLROOTDIR/gefusion
	chmod 755 $BININSTALLPROFILEDIR/ge-fusion.csh
	chmod 755 $BININSTALLPROFILEDIR/ge-fusion.sh

    check_fusion_master_or_slave

	if ! install_or_upgrade_asset_root; then
		exit 1
	fi

    fix_postinstall_filepermissions
    final_assetroot_configuration

    if ! final_fusion_service_configuration; then
        exit 1
    fi

    # install cleanup
    cleanup_installfiles
    show_final_success_message
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

show_intro()
{	
	echo -e "\nWelcome to the $GEEF $LONG_VERSION installer."
}

show_help()
{
	echo -e "\nUsage: \tsudo ./install_fusion.sh [-dir /tmp/fusion_os_install -ar /gevol/assets -sv /gevol/src -u fusionuser"
	echo -e "\t\t-g gegroup -nobk -nop -hnf -nostart]\n"

	echo -e "-h \t\tHelp - display this help screen"
	echo -e "-dir \t\tTemp Install Directory - specify the temporary install directory. Default is [$TMPINSTALLDIR]."	
	echo -e "-u \t\tFusion User Name - the user name to use for Fusion. Default is [$GEFUSIONUSER_NAME]. \n\t\tNote: this is only used for new installations."
	echo -e "-g \t\tUser Group Name - the group name to use for the Fusion user. Default is [$GROUPNAME]. \n\t\tNote: this is only used for new installations."
	echo -e "-ar \t\tAsset Root Mame - the name of the asset root volume.  Default is [$ASSET_ROOT]. \n\t\tNote: this is only used for new installations. Specify absolute paths only."
    echo -e "-sv \t\tSource Volume Name - the name of the source volume.  Default is [$SOURCE_VOLUME]. \n\t\tNote: this is only used for new installations. Specify absolute paths only."
	echo -e "-nobk \t\tNo Backup - do not backup the current fusion setup. Default is to backup \n\t\tthe setup before installing."
	echo -e "-nop \t\tNo Purge - do not delete the temporary install directory upon successful run of the installer."
	echo -e "\t\tDefault is to delete the directory after successful installation."
    echo -e "-nostart \tDo Not Start Fusion - after install, do not start the Fusion daemon.  Default is to start the daemon."
	echo -e "-hnf \t\tHostname Force - force the installer to continue installing with a bad \n\t\thostname. Bad hostname values are [${BADHOSTNAMELIST[*]}]."
	echo -e "-hnmf \t\tHostname Mismatch Force - force the installer to continue installing with a \n\t\tmismatched hostname.\n" 	
}

show_fusion_running_message()
{
	echo -e "\n$GEEF has active running processes."
	echo -e "To use this installer to upgrade, you must stop all fusion services.\n"	
}

load_systemrc_config()
{
	if [ -f "$SYSTEMRC" ]; then
		# read existing config information
		ASSET_ROOT=$(xmllint --xpath '//Systemrc/assetroot/text()' $SYSTEMRC)
		GEFUSIONUSER_NAME=$(xmllint --xpath '//Systemrc/fusionUsername/text()' $SYSTEMRC)
		GROUPNAME=$(xmllint --xpath '//Systemrc/userGroupname/text()' $SYSTEMRC)		
	fi
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
        elif [[ "${test_os,,}" == "red hat"* ]]; then
            MACHINE_OS=$REDHATKEY
        else
            MACHINE_OS=""
            echo -e "\nThe installer could not determine your machine's operating system."
            echo -e "Supported Operating Systems: ${SUPPORTED_OS_LIST[*]}\n"
            retval=1
        fi
    else
		echo -e "\nThe installer could not determine your machine's operating system."
        echo -e "Missing file: $OS_RELEASE\n"
        retval=1		
	fi

    return $retval
}

software_check()
{
	local software_check_retval=0
	
	# args: $1: ubuntu package
	# args: $: rhel package

    if [ "$MACHINE_OS" == "$UBUNTUKEY" ]; then
        if [[ -z "$(dpkg --get-selections | sed s:install:: | sed -e 's:\s::g' | grep ^$1\$)" ]]; then
            echo -e "Install $1 and restart the $GEEF $LONG_VERSION uninstaller."
            software_check_retval=1
        fi
    elif [ "$MACHINE_OS" == "$REDHATKEY" ]; then
        if [[ -z "$(rpm -qa | grep ^$2\$)" ]]; then
            echo -e "Install $2 and restart the $GEEF $LONG_VERSION uninstaller."
            software_check_retval=1
        fi
	else 
		echo -e "\nThe installer could not determine your machine's operating system."
            echo -e "Supported Operating Systems: ${SUPPORTED_OS_LIST[*]}\n"
            software_check_retval=1
    fi

	return $software_check_retval
}

check_prereq_software()
{
	local check_prereq_software_retval=0

	if ! software_check "libxml2-utils" "libxml2.*x86_64"; then
		check_prereq_software_retval=1
	fi

	if ! software_check "python2.7" "python-2.7.*"; then
		check_prereq_software_retval=1
	fi

	if [ -z "$(type -p X)" ] && [ ! -z "$(type -p Xorg)" ]; then
		show_X11
		check_prereq_software_retval=1
	fi

	return $check_prereq_software_retval
}

check_fusion_processes_running()
{
	check_fusion_processes_running_retval=0

	local manager_running=$(ps -e | grep gesystemmanager | grep -v grep)
	local res_provider_running=$(ps -ef | grep geresourceprovider | grep -v grep)

	if [ ! -z "$manager_running" ] || [ ! -z "$res_provider_running" ]; then
		check_fusion_processes_running_retval=1
	fi

	return $check_fusion_processes_running_retval
}

show_need_root()
{
	echo -e "\nYou must have root privileges to install $GEEF.\n"
	echo -e "Log in as the $ROOT_USERNAME user or use the 'sudo' command to run this installer."
	echo -e "The installer must exit."
}

show_invalid_assetroot_name()
(
	echo -e "\nThe following characters are no longer allowed in GEE Fusion Assets:"
	echo -e "& % \' \" * = + ~ \` ? < > : ; and the space character.\n"
	
	echo -e "Assets with these names will no longer be usable in GEE Fusion and will generate"
	echo -e "an appropriate error message.\n"

	echo -e "If you continue with installation, you will have to either recreate those assets using"
	echo -e "valid names or  rename the assets to a valid name before attempting to process the assets.\n"

	echo -e "See the GEE Admin Guide for more details on renaming assets.\n"

	echo -e "The following assets contain invalid characters:\n"
	echo -e "$1"
)

show_X11()
{
	echo -e "\nThe installer is not able to find the X11 package on this machine.\n"
	echo -e "Install the X11 package and restart the $GEEF installer.\n"
	echo -e "The installer must exit."
}

show_badhostname()
{
	echo -e "\nYour server [$HOSTNAME] contains an invalid hostname value which typically indicates an automatically generated"
	echo -e "hostname that might change over time.  A subsequent hostname change would cause configuration issues for the "
	echo -e "$GEEF software.  Invalid values: ${BADHOSTNAMELIST[*]}."
}

show_mismatchedhostname()
{
	echo -e "\nThe hostname of this machine does not match the fully-qualified hostname."
	echo -e "$GEEF requires that they match for local publishing to function properly."
	echo -e "To continue, the installer will update the hostname to match "
	echo -e "the fully-qualified hostname."
}

is_valid_custom_directory()
{
	# Standard function that tests a string to see if it passes the "valid" alphanumeric for asset root/source volume.
	# For simplicity -- we limit to letters, numbers and underscores.
	# Regular expression: 
	regex="^/([a-zA-Z0-9_]+/{0,1})+$"

	if [ ! -z "$1" ] && [[ $1 =~ $regex ]]; then
		return 0
	else
		return 1
	fi
}

is_valid_alphanumeric()
{
	# Standard function that tests a string to see if it passes the "valid" alphanumeric for user name and group name.
	# For simplicity -- we limit to letters, numbers and underscores.
	regex="^[a-zA-Z_]{1}[a-zA-Z0-9_-]*$"

	if [ ! -z "$1" ] && [[ $1 =~ $regex ]]; then
		return 0
	else
		return 1
	fi
}

show_no_tmp_dir_message()
{
	echo -e "\nThe temp install directory specified [$1] does not exist."
	echo -e "Please specify the path of the extracted install files.\n"
}

parse_arguments()
{
	local parse_arguments_retval=0
    local show_user_group_recommendation=false

	while [ $# -gt 0 ]
	do
		case "${1,,}" in
			-h|-help|--help)
				show_help
				parse_arguments_retval=1
				break
				;;
			-nobk)
				BACKUPFUSION=false				
				;;
			-hnf)
				BADHOSTNAMEOVERRIDE=true;
				;;			
			-hnmf)
				MISMATCHHOSTNAMEOVERRIDE=true
				;;
            -nostart)
                START_FUSION_DAEMON=false
                ;;
			-nop)
				PURGE_TMP_DIR=false
				;;
			-dir)
				shift
					if [ -d "$1" ]; then
						TMPINSTALLDIR="$1"
					else
						show_no_tmp_dir_message $1
						parse_arguments_retval=-1
						break		
					fi
					;;
			-ar)
				if [ $IS_NEWINSTALL == false ]; then
					echo -e "\nYou cannot modify the asset root using the installer because Fusion is already installed on this server."
					parse_arguments_retval=1
					break
				else
					shift

					if is_valid_custom_directory ${1// }; then
						ASSET_ROOT=${1// }
					else
						echo -e "\nThe asset root you specified does not appear to be a syntactically valid directory. Please"
						echo -e "specify the absolute path of a valid directory location. Valid characters are upper/lowercase"
						echo -e "letters, numbers and the underscore characters for the asset root name. The asset root cannot"
						echo -e "start with a number or underscore."
						parse_arguments_retval=1
						break		
					fi
				fi
				;;
            -sv)
                
                if [ $IS_NEWINSTALL == false ]; then
					echo -e "\nYou cannot modify the source volume using the installer because Fusion is already installed on this server."
					parse_arguments_retval=1
					break
				else
					shift

					if is_valid_directory ${1// }; then
						SOURCE_VOLUME=${1// }
					else
						echo -e "\nThe source volume you specified does not appear to be a syntactically valid directory. Please"
						echo -e "specify the absolute path of a valid directory location. Valid characters are upper/lowercase"
						echo -e "letters, numbers and the underscore characters for the source volume name. The source volume cannot"
						echo -e "start with a number or underscore."
						parse_arguments_retval=1
						break		
					fi
				fi
                ;;
			-u)
                show_user_group_recommendation=true

				if [ $IS_NEWINSTALL == false ]; then
					echo -e "\nYou cannot modify the fusion user name using the installer because Fusion is already installed on this server."
					parse_arguments_retval=1
					break
				else
					shift
				
					if is_valid_alphanumeric ${1// }; then
						GEFUSIONUSER_NAME=${1// }
					else
						echo -e "\nThe fusion user name you specified is not valid. Valid characters are upper/lowercase letters, "
						echo -e "numbers, dashes and the underscore characters. The user name cannot start with a number or dash."
						parse_arguments_retval=1					
						break
					fi
				fi
				;;
			-g)
                show_user_group_recommendation=true

				if [ $IS_NEWINSTALL == false ]; then
					echo -e "\nYou cannot modify the fusion user group using the installer because Fusion is already installed on this server."
					parse_arguments_retval=1
					break
				else
					shift
				
					if is_valid_alphanumeric ${1// }; then
						GROUPNAME=${1// }
					else
						echo -e "\nThe fusion group name you specified is not valid. Valid characters are upper/lowercase letters, "
						echo -e "numbers, dashes and the underscore characters. The group name cannot start with a number or dash."
						parse_arguments_retval=1
						break			
					fi
				fi
				;;
			*)
				echo -e "\nArgument Error: $1 is not a valid argument."
				show_help
				parse_arguments_retval=1
				break
				;;
		esac
	
		if [ $# -gt 0 ]
		then
		    shift
		fi
	done	

    # final check -- make sure that the asset root and source volume are not the same
    if [ "$ASSET_ROOT" == "$SOURCE_VOLUME" ]; then
        echo -e "\nArgument Error: The asset root cannot be the same as the source volume."
        echo -e "\nAsset Root: \t\t$ASSET_ROOT"
        echo -e "Source Volume: \t\t$SOURCE_VOLUME"
        parse_arguments_retval=1
    fi

    # show recommended label if the user attempts to change the default fusion user or group
    if [ $show_user_group_recommendation == true ]; then
        echo -e "\nRECOMMENDED:"
        echo -e "\nIt is strongly recommended that you use the default values for the fusion username and group."
        echo -e "\nDefault Fusion User: \t\t\t\t$DEFAULTGEFUSIONUSER_NAME"
	    echo -e "Default Fusion User Group: \t\t\t$DEFAULTGROUPNAME"
        echo -e "----------------"
        echo -e "Selected Fusion User: \t\t\t\t$GEFUSIONUSER_NAME"
	    echo -e "Selected Fusion User Group: \t\t\t$GROUPNAME"
    
        # START WORK HERE
        if ! prompt_to_quit "X (Exit) the installer and change the asset root location - C (Continue) to use the asset root that you have specified."; then
            parse_arguments_retval=1
        fi  
    fi
	
	return $parse_arguments_retval;
}

prompt_install_confirmation()
{
	local backupStringValue=""

	if [ $BACKUPFUSION == true ]; then
		backupStringValue="YES"
	else
		backupStringValue="NO"
	fi

	echo -e "\nYou have chosen to install $GEEF with the following settings:\n"
	echo -e "# CPU's: \t\t$NUM_CPUS"
	echo -e "Operating System: \t$MACHINE_OS_FRIENDLY"
	echo -e "64 bit OS: \t\tYES"	# Will always be true because we exit if 32 bit OS
	echo -e "Backup Fusion: \t\t$backupStringValue"
	echo -e "Install Location: \t$BASEINSTALLDIR_OPT"
	echo -e "Asset Root: \t\t$ASSET_ROOT"
    echo -e "Source Volume: \t\t$SOURCE_VOLUME"
	echo -e "Fusion User: \t\t$GEFUSIONUSER_NAME"
	echo -e "Fusion User Group: \t$GROUPNAME"
    echo -e "Disk Space:\n"
	
	# display disk space
	df -h | grep -v -E "^none"

	echo ""

	if ! prompt_to_quit "X (Exit) the installer and cancel the installation - C (Continue) to install/upgrade."; then
		return 1	
	else
        echo -e "\nProceeding with installation..."
		return 0
    fi
}

pause()
{
	printf "Press <ENTER> to continue..."
	read -r continueKey
}

backup_fusion()
{
	export BACKUP_DIR=$BACKUP_DIR

	if [ ! -d $BACKUP_DIR ]; then
		mkdir -p $BACKUP_DIR
	fi

	# copy log files.
	mkdir -p $BACKUP_DIR/log

	if [ -f "$GENERAL_LOG/gesystemmanager.log" ]; then 
		cp -f $GENERAL_LOG/gesystemmanager.log $BACKUP_DIR/log
	fi

	if [ -f "$GENERAL_LOG/geresourceprovider.log" ]; then 
		cp -f $GENERAL_LOG/geresourceprovider.log $BACKUP_DIR/log
	fi

	if [ -d "$BASEINSTALLDIR_ETC/openldap" ]; then 
		cp -rf $BASEINSTALLDIR_ETC/openldap $BACKUP_DIR
	fi

	if [ -f "$SYSTEMRC" ]; then 
		cp -f $SYSTEMRC $BACKUP_DIR
	fi

	# Change the ownership of the backup folder.
	chown -R $ROOT_USERNAME:$ROOT_USERNAME $BACKUP_DIR
}

#-----------------------------------------------------------------
# Install Functions
#-----------------------------------------------------------------
copy_files_to_target()
{
	printf "\nCopying files from source to target directories... "

	mkdir -p $BASEINSTALLDIR_OPT/bin
	mkdir -p $BASEINSTALLDIR_OPT/share/doc
	mkdir -p $BASEINSTALLDIR_OPT/share/gdal
	mkdir -p $BASEINSTALLDIR_OPT/share/fonts
	mkdir -p $BASEINSTALLDIR_OPT/gepython
	mkdir -p $BASEINSTALLDIR_OPT/qt
	mkdir -p $BASEINSTALLDIR_OPT/lib
	mkdir -p $BASEINSTALLDIR_VAR/openssl/private
	mkdir -p $BASEINSTALLDIR_VAR/openssl/misc
	mkdir -p $BASEINSTALLDIR_VAR/openssl/certs
	mkdir -p $BASEINSTALLDIR_ETC/openldap
	mkdir -p $BASEINSTALLDIR_VAR/run
	mkdir -p $BASEINSTALLDIR_VAR/log

	cp -rf $TMPINSTALLDIR/fusion/opt/google/bin $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/fusion/opt/google/share $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/common/opt/google/lib $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/fusion/opt/google/lib $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/common/opt/google/bin $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/common/opt/google/share $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/common/opt/google/qt $BASEINSTALLDIR_OPT
	
	cp -f $TMPINSTALLDIR/common/opt/google/qt/lib/* $BASEINSTALLDIR_OPT/lib
	cp -rf $TMPINSTALLDIR/common/opt/google/qt/lib/pkgconfig $BASEINSTALLDIR_OPT/lib
	cp -rf $TMPINSTALLDIR/common/opt/google/gepython $BASEINSTALLDIR_OPT
	cp -rf $TMPINSTALLDIR/manual/opt/google/share/doc/manual $BASEINSTALLDIR_OPT/share/doc

	cp -f $TMPINSTALLDIR/fusion/etc/profile.d/ge-fusion.csh $BININSTALLPROFILEDIR
	cp -f $TMPINSTALLDIR/fusion/etc/profile.d/ge-fusion.sh $BININSTALLPROFILEDIR
	cp -f $TMPINSTALLDIR/fusion/etc/init.d/gefusion $BININSTALLROOTDIR

	TMPOPENSSLPATH=$TMPINSTALLDIR/common/user_magic/var/opt/google/openssl

	cp -f $TMPOPENSSLPATH/openssl.cnf $BASEINSTALLDIR_VAR/openssl
	cp -rf $TMPOPENSSLPATH/private $BASEINSTALLDIR_VAR/openssl
	cp -f $TMPOPENSSLPATH/misc/CA.sh $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/tsget $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/c_name $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/CA.pl $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/c_issuer $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/c_info $BASEINSTALLDIR_VAR/openssl/misc
	cp -f $TMPOPENSSLPATH/misc/c_hash $BASEINSTALLDIR_VAR/openssl/misc
	cp -rf $TMPOPENSSLPATH/certs $BASEINSTALLDIR_VAR/openssl

	TMPOPENLDAPPATH=$TMPINSTALLDIR/common/user_magic/etc/opt/google/openldap

	cp -f $TMPOPENLDAPPATH/ldap.conf $BASEINSTALLDIR_ETC/openldap
	cp -f $TMPOPENLDAPPATH/ldap.conf.default $BASEINSTALLDIR_ETC/openldap

	# TODO: final step: copy uninstall script
	# cp -f $TMPOPENLDAPPATH/<........> $INSTALL_LOG_DIR

	printf "Done\n"

	printf "Copying Fusion Tutorial files... "

	if [ -d "$TMPINSTALLDIR/tutorial" ]; then
		cp -rf $TMPINSTALLDIR/tutorial $BASEINSTALLDIR_OPT
		printf "Done\n"
	else
		printf "Not found - Skipping tutorial install\n"
	fi
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

	printf "Done\n"	
}

#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------
setup_fusion_daemon()
{
	# setup fusion daemon
	printf "Setting up the Fusion daemon...\n"

	test -f $CHKCONFIG && $CHKCONFIG --add gefusion
	test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE -f gefusion remove
	test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE gefusion start 90 2 3 4 5 . stop 10 0 1 6 .
	
	printf "Fusion daemon setup ... Done\n"
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
        echo -e "Exiting the Installer.\n"	
        prompt_to_quit_retval=1
    fi        

    return $prompt_to_quit_retval
}

create_system_main_directories()
{
    if [ -d "$ASSET_ROOT" ]; then        
        ASSET_ROOT_PREEXISTING=true
    else
        mkdir -p $ASSET_ROOT
    fi

    if [ -d "$SOURCE_VOLUME" ]; then
        SOURCE_VOLUME_PREEXISTING=true
    else
        mkdir -p $SOURCE_VOLUME
    fi
}

compare_asset_root_publishvolume()
{    
    local compare_assetroot_publishvolume_retval=0

    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/stream_space" ]; then
		PUBLISH_ROOT_VOLUME="$(‘cut -d' ' -f3 /opt/google/gehttpd/conf.d/stream_space | cut -d'/' -f2 | $NEWLINECLEANER)"

        if [-d "$ASSET_ROOT" ] && [ -d "$PUBLISH_ROOT_VOLUME" ]; then
            VOL_ASSETROOT=$(df $ASSET_ROOT | grep -v ^Filesystem | grep -Eo '^[^ ]+')
            VOL_PUBLISHED_ROOT_VOLUME=$(df $PUBLISH_ROOT_VOLUME | grep -v ^Filesystem | grep -Eo '^[^ ]+')
            
            if [ "$VOL_ASSETROOT" != "$VOL_PUBLISHED_ROOT_VOLUME" ]; then
                echo -e "\nYou have selected different volumes for 'Publish Root' and 'Asset Root'."
                echo -e "\nAsset Root: \t\t$ASSET_ROOT"
                echo -e "Publish Root Volume: $PUBLISH_ROOT_VOLUME"
                echo -e "\nIf the publish root and asset root are on different logical volumes, then publishing"
                echo -e "will require a copy of the database to be made to the publish root. Otherwise, the publish"
                echo -e "can be quickly accomplished with hard/soft links without taking the additional space "
                echo -e "needed for a full copy."
                echo ""
        
                if ! prompt_to_quit "X (Exit) the installer and change the asset root location - C (Continue) to use the asset root that you have specified."; then
                    compare_assetroot_publishvolume_retval=1
                fi                
            fi   
        fi
	fi

    return $compare_assetroot_publishvolume_retval
}

check_asset_root_volume_size()
{
    local check_asset_root_volume_size_retval=0

    ASSET_ROOT_VOLUME_SIZE=$(df --output=avail $ASSET_ROOT | grep -v Avail)
    
    if [[ $ASSET_ROOT_VOLUME_SIZE -lt MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB ]]; then
        MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB=$(expr $MIN_ASSET_ROOT_VOLUME_SIZE_IN_KB / 1024 / 1024)

        echo -e "\nThe asset root volume [$ASSET_ROOT] has only $ASSET_ROOT_VOLUME_SIZE KB available."
        echo -e "We recommend that an asset root directory have a minimum of $MIN_ASSET_ROOT_VOLUME_SIZE_IN_GB GB of free disk space."
        echo ""

        if ! prompt_to_quit "X (Exit) the installer and change the asset root location - C (Continue) to use the asset root that you have specified."; then
            check_asset_root_volume_size_retval=1
        fi
    fi

    return $check_asset_root_volume_size_retval
}

check_fusion_master_or_slave()
{
    if [ -f "$ASSET_ROOT/.config/volumes.xml" ]; then
        EXISTING_HOST=$(xmllint --xpath "//VolumeDefList/volumedefs/item[1]/host/text()" $ASSET_ROOT/.config/volumes.xml | $NEWLINECLEANER)

        case "$EXISTING_HOST" in
			$HOSTNAME_F|$HOSTNAME_A|$HOSTNAME_S|$HOSTNAME)
                IS_SLAVE=false
                ;;
            *)
                IS_SLAVE=true

                echo -e "\nThe asset root [$ASSET_ROOT] is owned by another Fusion host:  $EXISTING_HOST"
                echo -e "Installing $GEEF $LONG_VERSION in Grid Slave mode.\n"
                pause
                ;;
        esac
    fi
}

create_systemrc()
{
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>
<Systemrc>
  <assetroot>$ASSET_ROOT</assetroot>
  <maxjobs>$NUM_CPUS</maxjobs>
  <locked>0</locked>
  <fusionUsername>$GEFUSIONUSER_NAME</fusionUsername>
  <userGroupname>$GROUPNAME</userGroupname>
</Systemrc>" > $SYSTEMRC
}

install_or_upgrade_asset_root()
{
    local install_or_upgrade_asset_root_retval=0

    if [ -f "$SYSTEMRC" ]; then
        # systemrc exist -- remove it
        rm -f $SYSTEMRC
    fi

    # create new systemrc file
    create_systemrc

    chmod 644 $SYSTEMRC
    chown $GEFUSIONUSER_NAME:$GROUPNAME $SYSTEMRC

    if [ $IS_NEWINSTALL == true ]; then
        # new install -- make sure that this does not exist
        if [ -d "$ASSET_ROOT/.config/volumes.xml" ]; then
            # error -- should never get here
            echo -e "\nThis appears to be a new install, but upon further investigation, it appears"
            echo -e "that some components have been installed before. This may indicate that a previous"
            echo -e "install process did not complete successfully.  Please uninstall fusion and"
            echo -e "re-run this script."

            install_or_upgrade_asset_root_retval=1
        else
            $BASEINSTALLDIR_OPT/bin/geconfigureassetroot --new --noprompt --assetroot $ASSET_ROOT --srcvol $SOURCE_VOLUME
            chown -R $GEFUSIONUSER_NAME:$GROUPNAME $ASSET_ROOT
        fi
    else
        # upgrade asset root -- if this is a master
        if [ $IS_SLAVE == false ]; then
            # TODO: Verify this logic -- this is what is defined in the installer documentation, but need confirmation            
            if [ $NEW_GEGROUP == true ] || [ $NEW_GEFUSIONUSER == true ]; then
                NOCHOWN=""
                UPGRADE_MESSAGE="\nThe upgrade will fix permissions for the asset root and source volume. This may take a while.\n"
            else
                NOCHOWN="--nochown"
                UPGRADE_MESSAGE=""
            fi

            echo -e "The asset root must be upgraded to work with the current version of $GEEF $LONG_VERSION."
            echo -e "You cannot use an upgraded asset root with older versions of $GEEF. "
			echo -e "Consider backing up your asset root. $GEEF will warn you when"
            echo -e "attempting to run with a non-upgraded asset root."
            echo -e "$UPGRADE_MESSAGE"
                
            if ! prompt_to_quit "X (Exit) the installer and backup your asset root - C (Continue) to upgrade the asset root."; then
                install_or_upgrade_asset_root_retval=1
			else
				# Note: we don't want to do the recursive chown on the asset root unless absolutely necessary
            	$BASEINSTALLDIR_OPT/bin/geconfigureassetroot --fixmasterhost --noprompt  $NOCHOWN --assetroot $ASSET_ROOT
            	$BASEINSTALLDIR_OPT/bin/geupgradeassetroot --noprompt $NOCHOWN --assetroot $ASSET_ROOT   

				chown -R $GEFUSIONUSER_NAME:$GROUPNAME $ASSET_ROOT
            fi
        fi  
    fi

    return $install_or_upgrade_asset_root_retval
}

fix_postinstall_filepermissions()
{
    # Run    
    chmod 775 $BASEINSTALLDIR_OPT/run
    chmod 775 $BASEINSTALLDIR_VAR/run
    chown $ROOT_USERNAME:$GROUPNAME $BASEINSTALLDIR_OPT/run
    chown $ROOT_USERNAME:$GROUPNAME $BASEINSTALLDIR_VAR/run

    # Logs
    chmod 775 $BASEINSTALLDIR_VAR/log
    chmod 775 $BASEINSTALLDIR_OPT/log
    chown $ROOT_USERNAME:$GROUPNAME $BASEINSTALLDIR_VAR/log
    chown $ROOT_USERNAME:$GROUPNAME $BASEINSTALLDIR_OPT/log

    # Etc
    chmod 755 $BASEINSTALLDIR_ETC
    chmod 755 $BASEINSTALLDIR_OPT/etc
    chmod 644 $SYSTEMRC

    # Other folders
    chmod 755 $BASEINSTALLDIR_OPT
    chmod 755 $BASEINSTALLDIR_VAR
    chmod -R 755 $BASEINSTALLDIR_OPT/lib
    chmod -R 555 $BASEINSTALLDIR_OPT/bin
    chmod 755 $BASEINSTALLDIR_OPT
    
    # suid enabled
    chmod +s $BASEINSTALLDIR_OPT/bin/geserveradmin
    chmod -R 755 $BASEINSTALLDIR_OPT/qt
    chmod 755 $BASEINSTALLDIR_ETC/openldap

    # Share
    find $BASEINSTALLDIR_OPT/share -type d -exec chmod 755 {} \;
    find $BASEINSTALLDIR_OPT/share -type f -exec chmod 644 {} \;
    chown -R $ROOT_USERNAME:$ROOT_USERNAME $BASEINSTALLDIR_OPT/share
    chmod ugo+x $BASEINSTALLDIR_OPT/share/support/geecheck/geecheck.pl

    # Restrict permissions to uninstaller and installer logs
    chmod -R go-rwx $INSTALL_LOG_DIR
}

final_assetroot_configuration()
{
    if [ $IS_SLAVE == true ]; then
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --role slave --assetroot $ASSET_ROOT
    else
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --assetroot $ASSET_ROOT

		if [ -d "$BASEINSTALLDIR_OPT/share/tutorials" ]; then
			$BASEINSTALLDIR_OPT/bin/geconfigureassetroot --addvolume opt:$BASEINSTALLDIR_OPT/share/tutorials --noprompt --nochown
		fi
    fi
}

final_fusion_service_configuration()
{
    local final_fusion_service_configuration_retval=0

    chcon -t texrel_shlib_t $BASEINSTALLDIR_OPT/lib/*so*

    if [ $START_FUSION_DAEMON == true ]; then
        if [ -f "$FUSIONBININSTALL" ]; then
            $FUSIONBININSTALL start
        else
            echo -e "\nThe fusion service could not be found.  It appears that some components"
            echo -e "have been installed, but others failed to install. This indicates that a previous"
            echo -e "install process did not complete successfully.  Please uninstall fusion and"
            echo -e "re-run this script."
            final_fusion_service_configuration_retval=1
        fi
    fi

    return $final_fusion_service_configuration_retval
}

cleanup_installfiles()
{
	if [ $PURGE_TMP_DIR == true ]; then
		printf "\nPerforming cleanup..."
	
		# remove the temp files 
		rm -rf $TMPINSTALLDIR

		printf "Done\n"
	fi
}

show_final_success_message()
{
	local INSTALLED_OR_UPGRADED=""
	local USER_INSTALL_DIRECTORY=$BASEINSTALLDIR_OPT
	local SYSTEM_MGR_CHECK=$(ps -e | grep gesystemmanager | grep -v grep)
	local SYSTEM_RPROVIDER_CHECK=$(ps -ef | grep geresourceprovider | grep -v grep)

    if [ $IS_NEWINSTALL == true ]; then
        INSTALLED_OR_UPGRADED="installed"
    else
        INSTALLED_OR_UPGRADED="upgraded"
    fi

    echo -e "\n-------------------"
    echo -e "Congratulations! $GEEF $LONG_VERSION has been successfully $INSTALLED_OR_UPGRADED"
    echo -e "in [$USER_INSTALL_DIRECTORY].\n"

    if [ $START_FUSION_DAEMON == true ]; then
        if [ -z "$SYSTEM_MGR_CHECK" ] && [ $IS_SLAVE == false ] || [ -z "$SYSTEM_RPROVIDER_CHECK" ]; then
            # warning 1: fusion systemmanager is not running properly for fusion master (not relevant for slave installs)
			# OR warning 2: fusion system resource provider is not running
            echo -e "$GEEF daemon services are not running properly."
            echo -e "Please ensure that your asset root is selected using:"
            echo -e "\n$BASEINSTALLDIR_OPT/bin/geselectassetroot --assetroot $ASSET_ROOT"
            echo -e "\nand then start up the $GEEF daemons using:"
            echo -e "\n$FUSIONBININSTALL start"
        fi
    else
        echo -e "\nYou opted to not install the $GEEF daemon."
        echo -e "To run the fusion daemon, execute the following:"
        echo -e "\n$FUSIONBININSTALL start"
    fi

    echo ""
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------
mkdir -p $INSTALL_LOG_DIR
exec 2> $INSTALL_LOG

main_preinstall "$@"

#-----------------------------------------------------------------
# Install Main
#-----------------------------------------------------------------
main_install

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------
main_postinstall

