#!/bin/bash
#
# Copyright 2018-2020 The Open GEE Contributors
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

umask 002

#------------------------------------------------------------------------------
# Definitions
GEEF="$GEE_NAME Fusion"

#------------------------------------------------------------------------------
# Get system info values:
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"
HOSTNAME_S="$(hostname -s | $NEWLINECLEANER)"
HOSTNAME_A="$(hostname -a | $NEWLINECLEANER)"

NUM_CPUS="$(nproc)"

# config values

# additional variables
PUBLISH_ROOT_VOLUME=""

EXISTING_HOST=""
IS_SLAVE=false


#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_postinstall()
{
    create_system_main_directories

    compare_asset_root_publishvolume

    setup_fusion_daemon

    fix_file_permissions

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
    echo "Setting up the Fusion daemon..."

    add_service gefusion

    echo "Fusion daemon setup ... Done"
}

create_system_main_directories()
{
    mkdir -p "$ASSET_ROOT"
    mkdir -p "$SOURCE_VOLUME"
}

compare_asset_root_publishvolume()
{
    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/stream_space" ]; then
        PUBLISH_ROOT_VOLUME="$(cut -d' ' -f3 /opt/google/gehttpd/conf.d/stream_space | cut -d'/' -f2 | $NEWLINECLEANER)"

        if [ -d "$ASSET_ROOT" ] && [ -d "$PUBLISH_ROOT_VOLUME" ]; then
            VOL_ASSETROOT=$(df "$ASSET_ROOT" | grep -v ^Filesystem | grep -Eo '^[^ ]+')
            VOL_PUBLISHED_ROOT_VOLUME=$(df "$PUBLISH_ROOT_VOLUME" | grep -v ^Filesystem | grep -Eo '^[^ ]+')
        fi
    fi
}

check_fusion_master_or_slave()
{
    if [ -f "$ASSET_ROOT/.config/volumes.xml" ]; then
        EXISTING_HOST=$(xml_file_get_xpath "$ASSET_ROOT/.config/volumes.xml" "//VolumeDefList/volumedefs/item[1]/host/text()" | $NEWLINECLEANER)

        case "$EXISTING_HOST" in
            $HOSTNAME_F|$HOSTNAME_A|$HOSTNAME_S|$HOSTNAME)
                IS_SLAVE=false
                ;;
            *)
                IS_SLAVE=true

                echo -e "\nThe asset root [$ASSET_ROOT] is owned by another Fusion host:  $EXISTING_HOST"
                echo -e "Installing $GEEF $GEE_VERSION in Grid Slave mode.\n"
                ;;
        esac
    fi
}

create_systemrc()
{
    cat > "$SYSTEMRC" <<END
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<Systemrc>
  <assetroot>$ASSET_ROOT</assetroot>
  <maxjobs>$NUM_CPUS</maxjobs>
  <locked>0</locked>
  <fusionUsername>$GEFUSIONUSER</fusionUsername>
  <userGroupname>$GEGROUP</userGroupname>
</Systemrc>
END
}

install_or_upgrade_asset_root()
{
    if [ -f "$SYSTEMRC" ]; then
        rm -f "$SYSTEMRC"
    fi

    # create new systemrc file
    create_systemrc

    chmod 644 "$SYSTEMRC"
    chown "$GEFUSIONUSER:$GEGROUP" "$SYSTEMRC"

    if [ ! -d "$ASSET_ROOT/.config" ]; then
        "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --new --noprompt \
            --assetroot "$ASSET_ROOT" --srcvol "$SOURCE_VOLUME"
    else
        # Upgrade the asset root, if this is a Fusion master host.
        #   Fusion slaves access the same files over NFS, and they rely on the
        # master to keep proper confguration and file permissions.
        if [ "$IS_SLAVE" = "false" ]; then
            OWNERSHIP=`find "$ASSET_ROOT" -maxdepth 0 -printf "%g:%u"`
            if [ "$OWNERSHIP" != "$GEGROUP:$GEFUSIONUSER" ] ; then
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

            "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --fixmasterhost \
                --noprompt --assetroot $ASSET_ROOT
            # If `geconfigureassetroot` already updated ownership, don't do it again:
            "$BASEINSTALLDIR_OPT/bin/geupgradeassetroot" --noprompt \
                --assetroot "$ASSET_ROOT"
        fi
    fi
}

final_assetroot_configuration()
{
    if [ "$IS_SLAVE" = "true" ]; then
        "$BASEINSTALLDIR_OPT/bin/geselectassetroot" --role slave --assetroot "$ASSET_ROOT"
    else
        "$BASEINSTALLDIR_OPT/bin/geselectassetroot" --assetroot "$ASSET_ROOT"
         add_fusion_tutorial_volume
    fi
}

final_fusion_service_configuration()
{
    chcon -t texrel_shlib_t "$BASEINSTALLDIR_OPT"/lib/*so* ||
      echo "Warning: chcon labeling failed. SELinux is probably not enabled"

    service gefusion start
}

fix_file_permissions()
{
    chown "root:$GEGROUP" "$BASEINSTALLDIR_VAR/run"
    chown "root:$GEGROUP" "$BASEINSTALLDIR_VAR/log"
    chmod -R 555 "$BASEINSTALLDIR_OPT/bin"

    if [ ! -d "$BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER" ]; then
      mkdir -p "$BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER"
    fi
    chown -R "$GEFUSIONUSER:$GEGROUP" "$BASEINSTALLDIR_OPT/.users/$GEFUSIONUSER"

    # TODO: Disabled for now...
    #sgid enabled
    #chown "root:$GEGROUP" "$BASEINSTALLDIR_OPT/bin/fusion"
    #chmod g+s "$BASEINSTALLDIR_OPT/bin/fusion"
}

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------

main_postinstall
