#!/bin/bash
#
# Copyright 2018-2019 The Open GEE Contributors
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

umask 002

#-----------------------------------------------------------------
# Definitions
PUBLISHER_ROOT="/gevol/published_dbs"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
PGSQL="/var/opt/google/pgsql"
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
    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/cgi-bin/set_geecheck_config.py" ] ; then
        cd "$BASEINSTALLDIR_OPT/gehttpd/cgi-bin"
        python ./set_geecheck_config.py
    fi

    # 9) Restore portable globes symlink if it existed previously
    restore_portable_symlink
 
    #10) Restore Admin Console password if it exists
    if [ -f "/tmp/.htpasswd" ]; then
        mv -f "/tmp/.htpasswd" "$BASEINSTALLDIR_OPT/gehttpd/conf.d/"
    fi

    #11) done!
    service geserver start

}

#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------

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

    # Publish Root - note these are not recursive
    # Ownership and permissions of publish root content is handled by geconfigurepublishroot
    chmod 775 $PUBLISHER_ROOT/stream_space
    chmod 644 $PUBLISHER_ROOT/.config
    chmod 755 $PUBLISHER_ROOT

    SEARCH_OWNER=`find "$PUBLISHER_ROOT/search_space" -maxdepth 0 -printf "%g:%u"`
    STREAM_OWNER=`find "$PUBLISHER_ROOT/stream_space" -maxdepth 0 -printf "%g:%u"`
    if [ "$SEARCH_OWNER" != "$GEGROUP:$GEAPACHEUSER" -o "$STREAM_OWNER" != "$GEGROUP:$GEAPACHEUSER" ] ; then
        printf "WARNING: The installer detected the publish root may have incorrect permissions! \
After installation you may need to run \n\n\
sudo /opt/google/bin/geconfigurepublishroot --noprompt --chown --path=$PUBLISHER_ROOT\n\n"
    fi
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
    if [ -f "${SEARCH_EX_SCRIPT}" ]; then
      chmod 0755 "${SEARCH_EX_SCRIPT}"
    fi
    if [ -f /opt/google/share/tutorials/fusion/download_tutorial.sh ]; then
      chmod ugo+x /opt/google/share/tutorials/fusion/download_tutorial.sh
    fi
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

    if [ ! -d "${BASEINSTALLDIR_OPT}/.users/${GEPGUSER}" ]; then
      mkdir -p "${BASEINSTALLDIR_OPT}/.users/${GEPGUSER}"
    fi
    chown -R "${GEPGUSER}:${GEGROUP}" "${BASEINSTALLDIR_OPT}/.users/${GEPGUSER}"

    if [ ! -d "${BASEINSTALLDIR_OPT}/.users/${GEAPACHEUSER}" ]; then
      mkdir -p "${BASEINSTALLDIR_OPT}/.users/${GEAPACHEUSER}"
    fi
    chown -R "${GEAPACHEUSER}:${GEGROUP}" "${BASEINSTALLDIR_OPT}/.users/${GEAPACHEUSER}"
}

reset_pgdb()
{
    # a) Always do an upgrade of the psql db
    DB_BACKUP=""
    DB_BACKUP_LATEST="$(ls -td -- $PGSQL/data.backup_dump*/ | head -n 1)"
    echo "Latest backup: $DB_BACKUP_LATEST"
    if [ -d "$DB_BACKUP_LATEST" ]; then
        DB_BACKUP="$DB_BACKUP_LATEST"
        echo "Restoring data backup from: $DB_BACKUP"
    fi

    echo 2 | run_as_user "$GEPGUSER" "/opt/google/bin/geresetpgdb upgrade $DB_BACKUP"
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
    
    # c) Install SearchExample Database
    install_searchexample_database

    # d) Turn off examplesearch (will be turned on by Extra, if installed).
    #  If 'Extra' already installed, don't delete
    if [ ! -f "$SQLDIR/examplesearch_delete.sql" ]; then
        echo "# d) Turn off examplesearch"
        run_as_user "$GEPGUSER" "$BASEINSTALLDIR_OPT/bin/psql -q -d gesearch geuser -f $SQLDIR/examplesearch_2delete.sql"
    fi

    # e) Stop the PSQL Server
    echo "# e) Stop the PSQL Server"
    run_as_user "$GEPGUSER" "$PGSQL_PROGRAM -D $PGSQL_DATA stop"
}

restore_portable_symlink()
{
    if [ -L "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes_symlink" ]; then
        rm -rf "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes"
        mv "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes_symlink" "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes"
    fi
}

#-----------------------------------------------------------------
# Post-install Main
#-----------------------------------------------------------------

main_postinstall $@
