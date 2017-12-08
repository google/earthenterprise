#! /bin/bash
#
# Copyright 2017 The Open GEE Contributors
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

    chown "root:$GEGROUP" "$BASEINSTALLDIR_VAR/run"
    chown "root:$GEGROUP" "$BASEINSTALLDIR_VAR/log"

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
        chown -R "$GEFUSIONUSER:$GEGROUP" "$ASSET_ROOT"
    else
        # upgrade asset root -- if this is a master
        if [ "$IS_SLAVE" = false ]; then
            # TODO: Verify this logic -- this is what is defined in the
            # installer documentation, but needs confirmation
            keyvalue_file_get "$GEE_INSTALL_KV_PATH" gegroup_existed NEW_GEFUSIONUSER
            keyvalue_file_get "$GEE_INSTALL_KV_PATH" gefusionuser_existed NEW_GEFUSIONUSER
            if [ "$NEW_GEGROUP" = true ] || [ "$NEW_GEFUSIONUSER" = true ]; then
                NOCHOWN=""
                UPGRADE_MESSAGE="The upgrade will fix permissions for the asset root and source volume. This may take a while."
            else
                NOCHOWN="--nochown"
                UPGRADE_MESSAGE=""
            fi

            cat <<END

The asset root must be upgraded to work with the current version of $GEEF $GEE_VERSION.
You cannot use an upgraded asset root with older versions of $GEEF. 
Consider backing up your asset root. $GEEF will warn you when
attempting to run with a non-upgraded asset root.

$UPGRADE_MESSAGE
END
            
            # Note: we don't want to do the recursive chown on the asset root
            # unless absolutely necessary
            "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --fixmasterhost \
                --noprompt  $NOCHOWN --assetroot $ASSET_ROOT
            "$BASEINSTALLDIR_OPT/bin/geupgradeassetroot" --noprompt $NOCHOWN \
                --assetroot "$ASSET_ROOT"
            chown -R "$GEFUSIONUSER:$GEGROUP" "$ASSET_ROOT"
        fi  
    fi
}

final_assetroot_configuration()
{
    if [ "$IS_SLAVE" = true ]; then
        "$BASEINSTALLDIR_OPT/bin/geselectassetroot" --role slave --assetroot "$ASSET_ROOT"
    else
        "$BASEINSTALLDIR_OPT/bin/geselectassetroot" --assetroot "$ASSET_ROOT"

        "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --addvolume \
            "opt:$BASEINSTALLDIR_OPT/share/tutorials" --noprompt --nochown
        if [ $? -eq 255 ]; then
            cat <<END
The geconfigureassetroot utility has failed while attempting
to add the volume 'opt:$BASEINSTALLDIR_OPT/share/tutorials'.
This is probably because a volume named 'opt' already exists.
END
        fi
    fi
}

final_fusion_service_configuration()
{
    chcon -t texrel_shlib_t "$BASEINSTALLDIR_OPT"/lib/*so*

    service gefusion start
}

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------
main_postinstall
