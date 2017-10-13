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

#-----------------------------------------------------------------
BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"

# directory locations
BININSTALLROOTDIR="/etc/init.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"

# derived directories
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"

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
}

#-----------------------------------------------------------------

# config values
ASSET_ROOT="/gevol/assets"

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
	# check to see if GE Fusion processes are running
	if ! check_fusion_processes_running; then
		# STOP PROCESSES
	fi

	load_systemrc_config

	# check for invalid asset names
	INVALID_ASSETROOT_NAMES=$(find $ASSET_ROOT -type d -name "*[\\\&\%\'\"\*\=\+\~\`\?\<\>\:\; ]*" 2> /dev/null)

	if [ ! -z "$INVALID_ASSETROOT_NAMES" ]; then
		show_invalid_assetroot_name $INVALID_ASSETROOT_NAMES
	fi

	check_bad_hostname
    
	check_mismatched_hostname
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

load_systemrc_config()
{
	if [ -f "$SYSTEMRC" ]; then
		# read existing config information
		ASSET_ROOT=$(xmllint --xpath '//Systemrc/assetroot/text()' $SYSTEMRC)
		GEFUSIONUSER_NAME=$(xmllint --xpath '//Systemrc/fusionUsername/text()' $SYSTEMRC)
		GROUPNAME=$(xmllint --xpath '//Systemrc/userGroupname/text()' $SYSTEMRC)		
	fi
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


#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall "$@"
