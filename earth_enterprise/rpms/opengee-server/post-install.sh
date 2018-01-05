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

set +x
umask 002

#-----------------------------------------------------------------
# Definitions
PUBLISHER_ROOT="/gevol/published_dbs"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
PGSQL_DATA="/var/opt/google/pgsql/data"
PGSQL_LOGS="/var/opt/google/pgsql/logs"
PGSQL_PROGRAM="/opt/google/bin/pg_ctl"
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

    # 3) Upgrade Postgres config
    reset_pgdb    

    # 4) Creating Daemon thread for geserver
    setup_geserver_daemon

    # 5) Register publish root
    "$BASEINSTALLDIR_OPT/bin/geconfigurepublishroot" --noprompt "--path=$PUBLISHER_ROOT"

    # 6) Install the GEPlaces and SearchExample Databases
    install_search_databases

    # 7) Repeated step; Set permissions after geserver Start/Stop.
    fix_postinstall_filepermissions

    # 8) Run geecheck config script
    # If file ‘/opt/google/gehttpd/cgi-bin/set_geecheck_config.py’ exists:
    if [ -f "$GEE_CHECK_CONFIG_SCRIPT" ]; then
        cd "$BASEINSTALLDIR_OPT/gehttpd/cgi-bin"
        python ./set_geecheck_config.py
    fi

    #9) done!
    service geserver start
}

#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------

run_as_user()
{
    local use_su=`su $1 -c 'echo -n 1' 2> /dev/null  || echo -n 0`
    if [ "$use_su" -eq 1 ] ; then
        >&2 echo "cd / ;su $1 -c \"$2\""
        ( cd / ;su $1 -c "$2" )
    else
        >&2 echo "cd / ;sudo -u $1 $2"
        ( cd / ;sudo -u $1 $2 )
    fi
} 

configure_publish_root()
{
    # Update PUBLISHER_ROOT if geserver already installed
    local STREAM_SPACE="$GEHTTPD_CONF/stream_space"
    if [ -e $STREAM_SPACE ]; then
        PUBLISHER_ROOT=`cat "$STREAM_SPACE" |cut -d" " -f3 |sed 's/.\{13\}$//'`
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
    echo "$RPM_PACKAGE_VERSION" >/etc/opt/google/fusion_server_version

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
    chmod ugo+x /opt/google/share/searchexample/searchexample
    chmod ugo+x /opt/google/share/geplaces/geplaces
    chmod ugo+x /opt/google/share/support/geecheck/geecheck.pl
    chmod ugo+x /opt/google/share/support/geecheck/convert_to_kml.pl
    chmod ugo+x /opt/google/share/support/geecheck/find_terrain_pixel.pl
    chmod ugo+x /opt/google/share/support/geecheck/pg_check.pl
    chown -R root:root /opt/google/share

    # Disable cutter (default) during installation.
    /opt/google/bin/geserveradmin --disable_cutter

    # Restrict permissions to uninstaller and installer logs
    chmod -R go-rwx "$BASEINSTALLDIR_OPT/install"
}

reset_pgdb()
{
    # a) Always do an upgrade of the psql db
    echo 2 | run_as_user "$GEPGUSER" "/opt/google/bin/geresetpgdb upgrade"
    echo -e "upgrade done"

    # b) Check for Success of PostGresDb
    if [ -d "$PGSQL_DATA" ]; then
        # PostgreSQL install Success.
        echo -e "The PostgreSQL component is successfully installed."
    else
        # postgress reset/install failed.
        echo -e "Failed to create PostGresDb."
        echo -e "The PostgreSQL component failed to install."
    fi
}

setup_geserver_daemon()
{   
    # setup geserver daemon
    add_service geserver
}

install_search_databases()
{   
  # a) Start the PSQL Server
    echo "# a) Start the PSQL Server "
    run_as_user "$GEPGUSER" "$PGSQL_PROGRAM -D $PGSQL_DATA -l $PGSQL_LOGS/pg.log start -w"

    echo "# b) Install GEPlaces Database"
    # b) Install GEPlaces Database
    run_as_user "$GEPGUSER" "/opt/google/share/geplaces/geplaces create"
    
    echo "# c) Install SearchExample Database "
    # c) Install SearchExample Database
    run_as_user "$GEPGUSER" "/opt/google/share/searchexample/searchexample create"

    # d) Stop the PSQL Server
    echo "# d) Stop the PSQL Server"
    run_as_user "$GEPGUSER" "$PGSQL_PROGRAM -D $PGSQL_DATA stop"
}

#-----------------------------------------------------------------
# Post-install Main
#-----------------------------------------------------------------

main_postinstall $@
