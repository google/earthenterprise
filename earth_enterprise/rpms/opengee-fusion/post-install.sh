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

# versions and user names
GEE="Google Earth Enterprise"
GEEF="$GEE Fusion"
LONG_VERSION="5.2.1"
ROOT_USERNAME="root"

# directory locations
BININSTALLROOTDIR="/etc/init.d"
BININSTALLPROFILEDIR="/etc/profile.d"
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_ETC="/etc/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
CHKCONFIG="/sbin/chkconfig"

# derived directories
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"
FUSIONBININSTALL="$BININSTALLROOTDIR/gefusion"

#------------------------------------------------------------------------------
# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"
HOSTNAME_S="$(hostname -s | $NEWLINECLEANER)"
HOSTNAME_A="$(hostname -a | $NEWLINECLEANER)"

NUM_CPUS="$(grep processor /proc/cpuinfo | wc -l | $NEWLINECLEANER)"

# config values
ASSET_ROOT="/gevol/assets"
SOURCE_VOLUME="/gevol/src"
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
DEFAULTGROUPNAME="gegroup"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME
GROUPNAME=$DEFAULTGROUPNAME

# script arguments
START_FUSION_DAEMON=true

# additional variables
IS_NEWINSTALL=false
PUBLISH_ROOT_VOLUME=""

EXISTING_HOST=""
IS_SLAVE=false
NEW_GEFUSIONUSER=false
NEW_GEGROUP=false

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_postinstall()
{
    create_system_main_directories

    compare_asset_root_publishvolume

    setup_fusion_daemon

    # store fusion version
    echo $LONG_VERSION > $BASEINSTALLDIR_ETC/fusion_version

    chmod 755 $BININSTALLROOTDIR/gefusion
    chmod 755 $BININSTALLPROFILEDIR/ge-fusion.csh
    chmod 755 $BININSTALLPROFILEDIR/ge-fusion.sh

    check_fusion_master_or_slave

    install_or_upgrade_asset_root

    final_assetroot_configuration

    final_fusion_service_configuration
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

create_system_main_directories()
{
    mkdir -p $ASSET_ROOT
    mkdir -p $SOURCE_VOLUME
}

compare_asset_root_publishvolume()
{    
    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/stream_space" ]; then
        PUBLISH_ROOT_VOLUME="$(cut -d' ' -f3 /opt/google/gehttpd/conf.d/stream_space | cut -d'/' -f2 | $NEWLINECLEANER)"

        if [ -d "$ASSET_ROOT" ] && [ -d "$PUBLISH_ROOT_VOLUME" ]; then
            VOL_ASSETROOT=$(df $ASSET_ROOT | grep -v ^Filesystem | grep -Eo '^[^ ]+')
            VOL_PUBLISHED_ROOT_VOLUME=$(df $PUBLISH_ROOT_VOLUME | grep -v ^Filesystem | grep -Eo '^[^ ]+')
            
        fi
    fi
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
    if [ -f "$SYSTEMRC" ]; then
        rm -f $SYSTEMRC
    fi

    # create new systemrc file
    create_systemrc

    chmod 644 $SYSTEMRC
    chown $GEFUSIONUSER_NAME:$GROUPNAME $SYSTEMRC

    if [ $IS_NEWINSTALL == true ]; then
        $BASEINSTALLDIR_OPT/bin/geconfigureassetroot --new --noprompt --assetroot $ASSET_ROOT --srcvol $SOURCE_VOLUME
        chown -R $GEFUSIONUSER_NAME:$GROUPNAME $ASSET_ROOT
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

            echo -e "\nThe asset root must be upgraded to work with the current version of $GEEF $LONG_VERSION."
            echo -e "You cannot use an upgraded asset root with older versions of $GEEF. "
            echo -e "Consider backing up your asset root. $GEEF will warn you when"
            echo -e "attempting to run with a non-upgraded asset root."
            echo -e "$UPGRADE_MESSAGE"
                
            # Note: we don't want to do the recursive chown on the asset root unless absolutely necessary
            $BASEINSTALLDIR_OPT/bin/geconfigureassetroot --fixmasterhost --noprompt  $NOCHOWN --assetroot $ASSET_ROOT
            $BASEINSTALLDIR_OPT/bin/geupgradeassetroot --noprompt $NOCHOWN --assetroot $ASSET_ROOT
            chown -R $GEFUSIONUSER_NAME:$GROUPNAME $ASSET_ROOT
        fi  
    fi
}

final_assetroot_configuration()
{
    if [ $IS_SLAVE == true ]; then
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --role slave --assetroot $ASSET_ROOT
    else
        $BASEINSTALLDIR_OPT/bin/geselectassetroot --assetroot $ASSET_ROOT

        mkdir -p $BASEINSTALLDIR_OPT/share/tutorials
        $BASEINSTALLDIR_OPT/bin/geconfigureassetroot --addvolume opt:$BASEINSTALLDIR_OPT/share/tutorials --noprompt --nochown
        if [ $? -eq 255 ]; then
            echo -e "The geconfigureassetroot utility has failed on attempting"
            echo -e "to add the volume 'opt:$BASEINSTALLDIR_OPT/share/tutorials'."
            echo -e "This is probably due to a volume named 'opt' already exists."
        fi
    fi
}

final_fusion_service_configuration()
{
    chcon -t texrel_shlib_t $BASEINSTALLDIR_OPT/lib/*so*

    /etc/init.d/gefusion start
}

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------
main_postinstall
