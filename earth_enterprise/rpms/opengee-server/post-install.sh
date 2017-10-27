#! /bin/bash
#
# Copyright 2017 the Open GEE Contributors
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

#-----------------------------------------------------------------
# Versions and user names:
GEE="Google Earth Enterprise"
PUBLISHER_ROOT="/gevol/published_dbs"
#-----------------------------------------------------------------

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_postinstall()
{
    # 0) Configure publishing db, we do it post install...
    configure_publish_root

    # 1) Modify files, maybe should be templated
    modify_files

    # 2) Set Permissions Before Server Start/Stop
    fix_postinstall_filepermissions
}

#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------

configure_publish_root()
{
    # Update PUBLISHER_ROOT if geserver already installed
    local STREAM_SPACE="$GEHTTPD_CONF/stream_space"
    if [ -e $STREAM_SPACE ]; then
        PUBLISHER_ROOT=`cat $STREAM_SPACE |cut -d" " -f3 |sed 's/.\{13\}$//'`
    fi
}

# /etc/init.d/geserver maybe should be expanded from a template instead...
modify_files()
{
    # Replace IA users in a file if they are found.
    # this step not requied for the OSS installs, however
    # if there are non-OSS installs in the system, it might help cleanup those.

    # a) Search and replace the below variables in /etc/init.d/geserver.
    sed -i "s/IA_GEAPACHE_USER/$GEAPACHEUSER/" $BININSTALLROOTDIR/geserver
    sed -i "s/IA_GEPGUSER/$GEPGUSER/" $BININSTALLROOTDIR/geserver

    # b) Search and replace the file ‘/opt/google/gehttpd/conf/gehttpd.conf’
    sed -i "s/IA_GEAPACHE_USER/$GEAPACHEUSER/" /opt/google/gehttpd/conf/gehttpd.conf
    sed -i "s/IA_GEPGUSER/$GEPGUSER/" /opt/google/gehttpd/conf/gehttpd.conf
    sed -i "s/IA_GEGROUP/$GEGROUP/" /opt/google/gehttpd/conf/gehttpd.conf

    # c) Create a new file ‘/etc/opt/google/fusion_server_version’ and
    # add the below text to it.
    echo $RPM_PACKAGE_VERSION >/etc/opt/google/fusion_server_version

    # d) Create a new file ‘/etc/init.d/gevars.sh’ and prepend the below lines.
    echo -e "GEAPACHEUSER=$GEAPACHEUSER\nGEPGUSER=$GEPGUSER\nGEFUSIONUSER=$GEFUSIONUSER\nGEGROUP=$GEGROUP" >$BININSTALLROOTDIR/gevars.sh
}

# this too probably should be done inside the rpm...
fix_postinstall_filepermissions()
{
    # PostGres
    chown -R $GEPGUSER:$GEGROUP $BASEINSTALLDIR_VAR/pgsql/

    # Apache
    mkdir -p $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime
    chmod -R 755 $BASEINSTALLDIR_OPT/gehttpd
    chmod -R 775 $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime/
    chown -R $GEAPACHEUSER:$GEGROUP $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/
    chown -R $GEAPACHEUSER:$GEGROUP $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/
    chmod -R 700 $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes/
    chown $GEAPACHEUSER:$GEGROUP $BASEINSTALLDIR_OPT/gehttpd/htdocs/.htaccess
    chown -R $GEAPACHEUSER:$GEGROUP $BASEINSTALLDIR_OPT/gehttpd/logs

    # Publish Root
    chmod 775 $PUBLISHER_ROOT/stream_space
    # TODO - Not Found
    # chmod 644 $PUBLISHER_ROOT/stream_space/.config
    chmod 644 $PUBLISHER_ROOT/.config
    chmod 755 $PUBLISHER_ROOT
    chown -R $GEAPACHEUSER:$GEGROUP $PUBLISHER_ROOT/stream_space
    chown -R $GEAPACHEUSER:$GEGROUP $PUBLISHER_ROOT/search_space

    # Run and logs ownership
    chown root:$GEGROUP $BASEINSTALLDIR_OPT/run
    chown root:$GEGROUP $BASEINSTALLDIR_VAR/run
    chown root:$GEGROUP $BASEINSTALLDIR_VAR/log
    chown root:$GEGROUP $BASEINSTALLDIR_OPT/log

    # setuid requirements...hmmm
    chmod +s /opt/google/bin/gerestartapache /opt/google/bin/geresetpgdb /opt/google/bin/geserveradmin

    # Tutorial and Share
    find /opt/google/share -type d -exec chmod 755 {} \;
    find /opt/google/share -type f -exec chmod 644 {} \;
#TODO
#    chmod ugo+x /opt/google/share/searchexample/searchexample
#    chmod ugo+x /opt/google/share/geplaces/geplaces
    chmod ugo+x /opt/google/share/support/geecheck/geecheck.pl
    chmod ugo+x /opt/google/share/support/geecheck/convert_to_kml.pl
    chmod ugo+x /opt/google/share/support/geecheck/find_terrain_pixel.pl
    chmod ugo+x /opt/google/share/support/geecheck/pg_check.pl
    # Note: this is installed in install_fusion.sh, but needs setting here too.
#TODO
#    chmod ugo+x $BASEINSTALLDIR_OPT/share/tutorials/fusion/download_tutorial.sh
    # this should already be true...
    chown -R root:root /opt/google/share

    #TODO
    # Set context (needed for SELINUX support) for shared libs
    # chcon -t texrel_shlib_t /opt/google/lib/*so*
    # Restrict permissions to uninstaller and installer logs
    #chmod -R go-rwx /opt/google/install

    # Disable cutter (default) during installation.
    /opt/google/bin/geserveradmin --disable_cutter

    # Restrict permissions to uninstaller and installer logs
    chmod -R go-rwx $BASEINSTALLDIR_OPT/install
}

#-----------------------------------------------------------------
# Post-install Main
#-----------------------------------------------------------------

main_postinstall "$@"
