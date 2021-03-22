#!/bin/bash
#
# Copyright 2017 Google Inc.
# Copyright 2018-2021 Open GEE Contributors
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
umask 002

# get script directory
SCRIPTDIR=`dirname $0`

. $SCRIPTDIR/common.sh

# config values
ASSET_ROOT="/gevol/assets"
SOURCE_VOLUME="/gevol/src"
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
DEFAULTGROUPNAME="gegroup"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME
GROUPNAME=$DEFAULTGROUPNAME
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"

# script arguments
BACKUPFUSION=true
BADHOSTNAMEOVERRIDE=false
MISMATCHHOSTNAMEOVERRIDE=false
START_FUSION_DAEMON=true

INSTALL_LOG="$INSTALL_LOG_DIR/fusion_install_$(date +%Y_%m_%d.%H%M%S).log"
BACKUP_DIR="$BASEINSTALLDIR_VAR/fusion-backups/$(date +%Y_%m_%d.%H%sM%S)"

# additional variables
IS_NEWINSTALL=false
PUBLISH_ROOT_VOLUME=""

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
		BACKUPFUSION=false
	fi

	# Argument check
	if ! parse_arguments "$@"; then
		exit 1
	fi

	if ! determine_os; then
            exit 1
        fi

	if is_package_installed "opengee-common" "opengee-common" ; then
        show_opengee_package_installed "install" "$GEEF"
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

	if ! check_bad_hostname; then
		exit 1
	fi

	if ! check_mismatched_hostname; then
		exit 1
	fi

	# 64 bit check
	if [[ "$(uname -i)" != "x86_64" ]]; then
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
		useradd --home $BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER_NAME --system --gid $GROUPNAME $GEFUSIONUSER_NAME
		NEW_GEFUSIONUSER=true
	else
		# user already exists -- update primary group
		usermod -g $GROUPNAME $GEFUSIONUSER_NAME
	fi
        chown -R "GEFUSIONUSER_NAME:$GROUPNAME" "$BASEINSTALLDIR_OPT/.users/GEFUSIONUSER_NAME"

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
	echo -e "\t\t-g gegroup -nobk -hnf -nostart]\n"

	echo -e "-h \t\tHelp - display this help screen"
	echo -e "-dir \t\tTemp Install Directory - specify the temporary install directory. Default is [$TMPINSTALLDIR]."
	echo -e "-u \t\tFusion User Name - the user name to use for Fusion. Default is [$GEFUSIONUSER_NAME]. \n\t\tNote: this is only used for new installations."
	echo -e "-g \t\tUser Group Name - the group name to use for the Fusion user. Default is [$GROUPNAME]. \n\t\tNote: this is only used for new installations."
	echo -e "-ar \t\tAsset Root Name - the name of the asset root volume.  Default is [$ASSET_ROOT]. \n\t\tNote: this is only used for new installations. Specify absolute paths only."
    echo -e "-sv \t\tSource Volume Name - the name of the source volume.  Default is [$SOURCE_VOLUME]. \n\t\tNote: this is only used for new installations. Specify absolute paths only."
	echo -e "-nobk \t\tNo Backup - do not backup the current fusion setup. Default is to backup \n\t\tthe setup before installing."
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
		ASSET_ROOT=$(xml_file_get_xpath "$SYSTEMRC" "//Systemrc/assetroot/text()")
		GEFUSIONUSER_NAME=$(xml_file_get_xpath "$SYSTEMRC" "//Systemrc/fusionUsername/text()")
		GROUPNAME=$(xml_file_get_xpath "$SYSTEMRC" "//Systemrc/userGroupname/text()")
	fi
}

check_prereq_software()
{
	local check_prereq_software_retval=0
  local script_name="$GEEF $LONG_VERSION installer"

	if ! software_check "$script_name" "libxml2-utils" "libxml2.*x86_64"; then
		check_prereq_software_retval=1
	fi

	if ! software_check "$script_name" "python2.[67]" "python[2]*-2.[67].*"; then
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

					if is_valid_custom_directory ${1// }; then
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
					# Don't show the User Group dialog since it is invalid to change the fusion
					# username once fusion is installed on the server
					show_user_group_recommendation=false
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
					# Don't show the User Group dialog since it is invalid to change the fusion
					# username once fusion is installed on the server
					show_user_group_recommendation=false
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
        if ! prompt_to_quit "X (Exit) the installer and use the default username - C (Continue) to use the username that you have specified."; then
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
	if [ $IS_NEWINSTALL == false ]; then
		echo -e "Backup Fusion: \t\t$backupStringValue"
	fi
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
	printf "\nCopying files from source to target directories..."

	mkdir -p $BASEINSTALLDIR_OPT/bin
	mkdir -p $BASEINSTALLDIR_OPT/share/datasets
	mkdir -p $BASEINSTALLDIR_OPT/share/doc
	mkdir -p $BASEINSTALLDIR_OPT/share/gdal
	mkdir -p $BASEINSTALLDIR_OPT/share/fonts
	mkdir -p $BASEINSTALLDIR_OPT/gepython
	mkdir -p $BASEINSTALLDIR_OPT/lib
	mkdir -p $BASEINSTALLDIR_VAR/openssl/private
	mkdir -p $BASEINSTALLDIR_VAR/openssl/misc
	mkdir -p $BASEINSTALLDIR_VAR/openssl/certs
	mkdir -p $BASEINSTALLDIR_ETC/openldap
	mkdir -p $BASEINSTALLDIR_VAR/run
	mkdir -p $BASEINSTALLDIR_VAR/log

	local error_on_copy=0
	cp -rf $TMPINSTALLDIR/fusion/opt/google/bin $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/fusion/opt/google/share $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/common/opt/google/lib $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/fusion/opt/google/lib $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/common/opt/google/bin $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/common/opt/google/share $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi

	# copy "lib*" vs "*" because "cp *" will skip dir 'pkgconfig' and return error
	cp -rf $TMPINSTALLDIR/common/opt/google/gepython $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPINSTALLDIR/manual/opt/google/share/doc/manual/ $BASEINSTALLDIR_OPT/share/doc
	if [ $? -ne 0 ]; then error_on_copy=1; fi

	cp -f $TMPINSTALLDIR/fusion/etc/profile.d/ge-fusion.csh $BININSTALLPROFILEDIR
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPINSTALLDIR/fusion/etc/profile.d/ge-fusion.sh $BININSTALLPROFILEDIR
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPINSTALLDIR/fusion/etc/init.d/gefusion $BININSTALLROOTDIR

	TMPOPENSSLPATH=$TMPINSTALLDIR/common/user_magic/var/opt/google/openssl

	cp -f $TMPOPENSSLPATH/openssl.cnf $BASEINSTALLDIR_VAR/openssl
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPOPENSSLPATH/private $BASEINSTALLDIR_VAR/openssl
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/CA.sh $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/tsget $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/c_name $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/CA.pl $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/c_issuer $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/c_info $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENSSLPATH/misc/c_hash $BASEINSTALLDIR_VAR/openssl/misc
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -rf $TMPOPENSSLPATH/certs $BASEINSTALLDIR_VAR/openssl
	if [ $? -ne 0 ]; then error_on_copy=1; fi

	TMPOPENLDAPPATH=$TMPINSTALLDIR/common/user_magic/etc/opt/google/openldap

	cp -f $TMPOPENLDAPPATH/ldap.conf $BASEINSTALLDIR_ETC/openldap
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPOPENLDAPPATH/ldap.conf.default $BASEINSTALLDIR_ETC/openldap
	if [ $? -ne 0 ]; then error_on_copy=1; fi

	cp -f $TMPINSTALLDIR/common/opt/google/uninstall_fusion.sh $INSTALL_LOG_DIR
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPINSTALLDIR/common/opt/google/common.sh $INSTALL_LOG_DIR
	if [ $? -ne 0 ]; then error_on_copy=1; fi
	cp -f $TMPINSTALLDIR/common/opt/google/version.txt $BASEINSTALLDIR_OPT
	if [ $? -ne 0 ]; then error_on_copy=1; fi

	if [ $error_on_copy -ne 0 ]
	then
		show_corrupt_tmp_dir_message $TMPINSTALLDIR $INSTALL_LOG
		exit 1
	fi
	printf "Done\n"

	printf "Copying Fusion Tutorial files... "

	if [ -d "$TMPINSTALLDIR/tutorial" ]; then
		cp -rf $TMPINSTALLDIR/tutorial/opt/google/share $BASEINSTALLDIR_OPT
		printf "Done\n"
	else
		printf "Not found - Skipping tutorial install\n"
	fi
}

#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------
setup_fusion_daemon()
{
	# setup fusion daemon
	printf "Setting up the Fusion daemon...\n"

    # Create a new file ‘/etc/init.d/gevars.sh’ and add the below lines.
    echo -e "GEAPACHEUSER=$GEAPACHEUSER_NAME\nGEPGUSER=$GEPGUSER_NAME\nGEFUSIONUSER=$GEFUSIONUSER_NAME\nGEGROUP=$GROUPNAME" > $BININSTALLROOTDIR/gevars.sh

	test -f $CHKCONFIG && $CHKCONFIG --add gefusion
	test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE -f gefusion remove
	test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE gefusion start 90 2 3 4 5 . stop 10 0 1 6 .

	printf "Fusion daemon setup ... Done\n"
}

create_system_main_directories()
{
    if [ ! -d "$ASSET_ROOT" ]; then
        mkdir -p $ASSET_ROOT
    fi

    if [ ! -d "$SOURCE_VOLUME" ]; then
        mkdir -p $SOURCE_VOLUME
    fi
}

compare_asset_root_publishvolume()
{
    local compare_assetroot_publishvolume_retval=0

    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/stream_space" ]; then
		PUBLISH_ROOT_VOLUME="$(cut -d' ' -f3 /opt/google/gehttpd/conf.d/stream_space | cut -d'/' -f2 | $NEWLINECLEANER)"

        if [ -d "$ASSET_ROOT" ] && [ -d "$PUBLISH_ROOT_VOLUME" ]; then
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

        if ! prompt_to_quit "X (Exit) the installer and change the asset root location with larger volume - C (Continue) to use the asset root that you have specified."; then
            check_asset_root_volume_size_retval=1
        fi
    fi

    return $check_asset_root_volume_size_retval
}

check_fusion_master_or_slave()
{
    if [ -f "$ASSET_ROOT/.config/volumes.xml" ]; then
	EXISTING_HOST=$(xml_file_get_xpath "$ASSET_ROOT/.config/volumes.xml" "//VolumeDefList/volumedefs/item[1]/host/text()")
	EXISTING_HOST=`echo "$EXISTING_HOST" | $NEWLINECLEANER`
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

    if [ ! -d "$ASSET_ROOT/.config" ]; then
        "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --new --noprompt \
            --assetroot "$ASSET_ROOT" --srcvol "$SOURCE_VOLUME"
    else
        # Upgrade the asset root, if this is a Fusion master host.
        #   Fusion slaves access the same files over NFS, and they rely on the
        # master to keep proper confguration and file permissions.
        if [ "$IS_SLAVE" = "false" ]; then
            OWNERSHIP=`find "$ASSET_ROOT" -maxdepth 0 -printf "%g:%u"`
            if [ "$OWNERSHIP" != "$GROUPNAME:$GEFUSIONUSER_NAME" ] ; then
                UPGRADE_MESSAGE="WARNING: The installer detected the asset root may have incorrect permissions! \
After installation you may need to run \n\n\
sudo $BASEINSTALLDIR_OPT/bin/geconfigureassetroot --noprompt --chown --repair --assetroot $ASSET_ROOT\n\n"
            else
                UPGRADE_MESSAGE=""
            fi

            cat <<END

The asset root must be upgraded to work with the current version of $GEEF $GEE_VERSION.
You cannot use an upgraded asset root with older versions of $GEEF.
Consider backing up your asset root. $GEEF will warn you when
attempting to run with a non-upgraded asset root.

END
            if [ ! -z "$UPGRADE_MESSAGE" ]; then 
                printf "$UPGRADE_MESSAGE"
            fi

            if ! prompt_to_quit "X (Exit) the installer and backup your asset root - C (Continue) to upgrade the asset root."; then
                install_or_upgrade_asset_root_retval=1
			else
            	$BASEINSTALLDIR_OPT/bin/geconfigureassetroot --fixmasterhost --noprompt --assetroot $ASSET_ROOT
            	$BASEINSTALLDIR_OPT/bin/geupgradeassetroot --noprompt --assetroot $ASSET_ROOT
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
    chmod 755 "$BININSTALLROOTDIR/gevars.sh"

    # Other folders
    chmod 755 $BASEINSTALLDIR_OPT
    chmod 755 $BASEINSTALLDIR_VAR
    chmod -R 755 $BASEINSTALLDIR_OPT/lib
    chmod -R 555 $BASEINSTALLDIR_OPT/bin
    chmod 755 $BASEINSTALLDIR_OPT

    # suid enabled
    chmod +s $BASEINSTALLDIR_OPT/bin/geserveradmin
    chmod 755 $BASEINSTALLDIR_ETC/openldap

    # TODO: resolve access issues
    #sgid disabled
    #chown $ROOT_USERNAME:$GROUPNAME $BASEINSTALLDIR_OPT/bin/fusion
    #chmod g+s $BASEINSTALLDIR_OPT/bin/fusion

    # Share
    find $BASEINSTALLDIR_OPT/share -type d -exec chmod 755 {} \;
    find $BASEINSTALLDIR_OPT/share -type f -exec chmod 644 {} \;
    chown -R $ROOT_USERNAME:$ROOT_USERNAME $BASEINSTALLDIR_OPT/share
    chmod ugo+x $BASEINSTALLDIR_OPT/share/support/geecheck/geecheck.pl
    chmod ugo+x $BASEINSTALLDIR_OPT/share/tutorials/fusion/download_tutorial.sh

    # Restrict permissions to uninstaller and installer logs
    chmod -R go-rwx $INSTALL_LOG_DIR
}

final_assetroot_configuration()
{
    if [ $IS_SLAVE == true ]; then
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --role slave --assetroot $ASSET_ROOT
    else
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --assetroot $ASSET_ROOT

	mkdir -p $BASEINSTALLDIR_OPT/share/tutorials
	$BASEINSTALLDIR_OPT/bin/geconfigureassetroot --addvolume opt:$BASEINSTALLDIR_OPT/share/tutorials --noprompt
	if [ $? -eq 255 ]; then
            echo -e "The geconfigureassetroot utility has failed on attempting"
            echo -e "to add the volume 'opt:$BASEINSTALLDIR_OPT/share/tutorials'."
            echo -e "This is probably due to a volume named 'opt' already exists."
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
    echo -e "in [$USER_INSTALL_DIRECTORY]."
    echo -e "You can remove the staged install files from $TMPINSTALLDIR if you do not"
    echo -e "need to install $GEES."

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

