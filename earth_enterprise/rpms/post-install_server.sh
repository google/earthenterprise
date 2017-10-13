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
# from common.sh:

BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"
LONG_VERSION="5.2.1"

MACHINE_OS=""

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

SUPPORTED_OS_LIST=("Ubuntu", "Red Hat Enterprise Linux (RHEL)", "CentOS", "Linux Mint")
UBUNTUKEY="ubuntu"
REDHATKEY="rhel"
CENTOSKEY="centos"

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
  echo "wsgi service: $gehttpd_running_str"

  return $retval
}


#-----------------------------------------------------------------

# config values
PUBLISHER_ROOT="/gevol/published_dbs"
DEFAULTGROUPNAME="gegroup"
GROUPNAME=$DEFAULTGROUPNAME
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME

# user names
GRPNAME="gegroup"
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"

SERVER_INSTALL_OR_UPGRADE="install"
GEE_CHECK_CONFIG_SCRIPT="/opt/google/gehttpd/cgi-bin/set_geecheck_config.py"
PGSQL_DATA="/var/opt/google/pgsql/data"
PGSQL_LOGS="/var/opt/google/pgsql/logs"
PGSQL_PROGRAM="/opt/google/bin/pg_ctl"
PRODUCT_NAME="$GEE"

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------

main_postinstall()
{
  # 1) Modify files
  modify_files

  # 2) Set Permissions Before Server Start/Stop
  fix_postinstall_filepermissions

  # 3) PostGres DB config
  postgres_db_config

  # 4) Creating Daemon thread for geserver
  setup_geserver_daemon

  # 5) Add the server to services
  # For RedHat and SuSE
  test -f $CHKCONFIG && $CHKCONFIG --add geserver

  # 6) Register publish root
  $BASEINSTALLDIR_OPT/bin/geconfigurepublishroot --noprompt --path=$PUBLISHER_ROOT

  # 7) Install the GEPlaces and SearchExample Databases
  install_search_databases

  # 8) Set permissions after geserver Start/Stop.
  fix_postinstall_filepermissions

  # TODO - verify
  # 9) Run geecheck config script
  # If file ‘/opt/google/gehttpd/cgi-bin/set_geecheck_config.py’ exists:
  if [ -f "$GEE_CHECK_CONFIG_SCRIPT" ]; then
    cd $BASEINSTALLDIR_OPT/gehttpd/cgi-bin
    python ./set_geecheck_config.py
  fi

  # 10)
  show_final_success_message
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

#-----------------------------------------------------------------
# Install Functions
#-----------------------------------------------------------------
#-----------------------------------------------------------------
# Post-install Functions
#-----------------------------------------------------------------

postgres_db_config()
{
  # a) PostGres folders and configuration
  chmod -R 700 /var/opt/google/pgsql/
  chown -R $GEPGUSER_NAME:$GRPNAME /var/opt/google/pgsql/
  reset_pgdb
}

reset_pgdb()
{
  # TODO check if correct
  # a) Always do an upgrade of the psql db
  echo 2 | run_as_user $GEPGUSER_NAME "/opt/google/bin/geresetpgdb upgrade"
  echo -e "upgrade done"

  # b) Check for Success of PostGresDb
  #  If file ‘/var/opt/google/pgsql/data’ doesn’t exist:

  echo  "pgsql_data"
  echo $PGSQL_DATA
  if [ -d "$PGSQL_DATA" ]; then
    # PostgreSQL install Success.
    echo -e "The PostgreSQL component is successfully installed."
  else
    # postgress reset/install failed.
    echo -e "Failed to create PostGresDb."
    echo -e "The PostgreSQL component of the installation failed
         to install."
  fi
}

# 4) Creating Daemon thread for geserver
setup_geserver_daemon()
{
  # setup geserver daemon
  # For Ubuntu
  printf "Setting up the geserver daemon...\n"
  test -f $CHKCONFIG && $CHKCONFIG --add geserver
  test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE -f geserver remove
  test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE geserver start 90 2 3 4 5 . stop 10 0 1 6 .
  printf "GEE Server daemon setup ... DONE\n"
}

# 6) Install the GEPlaces and SearchExample Databases
install_search_databases()
{
  # a) Start the PSQL Server
  echo "# a) Start the PSQL Server "
  run_as_user $GEPGUSER_NAME "$PGSQL_PROGRAM -D $PGSQL_DATA -l $PGSQL_LOGS/pg.log start -w"

  echo "# b) Install GEPlaces Database"
  # b) Install GEPlaces Database
  run_as_user $GEPGUSER_NAME "/opt/google/share/geplaces/geplaces create"

  echo "# c) Install SearchExample Database "
  # c) Install SearchExample Database
  run_as_user $GEPGUSER_NAME "/opt/google/share/searchexample/searchexample create"

  # d) Stop the PSQL Server
  echo "# d) Stop the PSQL Server"
  run_as_user $GEPGUSER_NAME "$PGSQL_PROGRAM -D $PGSQL_DATA stop"
}

modify_files()
{
  # Replace IA users in a file if they are found.
  # this step not requied for the OSS installs, however
  # if there are non-OSS installs in the system, it might help cleanup those.

  # a) Search and replace the below variables in /etc/init.d/geserver.
  sed -i "s/IA_GEAPACHE_USER/$GEAPACHEUSER_NAME/" $BININSTALLROOTDIR/geserver
  sed -i "s/IA_GEPGUSER/$GEPGUSER_NAME/" $BININSTALLROOTDIR/geserver

  # b) Search and replace the file ‘/opt/google/gehttpd/conf/gehttpd.conf’
  sed -i "s/IA_GEAPACHE_USER/$GEAPACHEUSER_NAME/" /opt/google/gehttpd/conf/gehttpd.conf
  sed -i "s/IA_GEPGUSER/$GEPGUSER_NAME/" /opt/google/gehttpd/conf/gehttpd.conf
  sed -i "s/IA_GEGROUP/$GRPNAME/" /opt/google/gehttpd/conf/gehttpd.conf

  # c) Create a new file ‘/etc/opt/google/fusion_server_version’ and
  # add the below text to it.
  echo $LONG_VERSION > /etc/opt/google/fusion_server_version

  # d) Create a new file ‘/etc/init.d/gevars.sh’ and prepend the below lines.
  echo -e "GEAPACHEUSER=$GEAPACHEUSER_NAME\nGEPGUSER=$GEPGUSER_NAME\nGEFUSIONUSER=$GEFUSIONUSER_NAME\nGEGROUP=$GRPNAME" > $BININSTALLROOTDIR/gevars.sh
}

fix_postinstall_filepermissions()
{
  # PostGres
  chown -R $GEPGUSER_NAME:$GRPNAME $BASEINSTALLDIR_VAR/pgsql/

  # Apache
  mkdir -p $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime
  chmod -R 755 $BASEINSTALLDIR_OPT/gehttpd
  chmod -R 775 $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime/
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/
  chmod -R 700 $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes/
  chown $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/htdocs/.htaccess
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/logs

  # Publish Root
  chmod 775 $PUBLISHER_ROOT/stream_space
  # TODO - Not Found
  # chmod 644 $PUBLISHER_ROOT/stream_space/.config
  chmod 644 $PUBLISHER_ROOT/.config
  chmod 755 $PUBLISHER_ROOT
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $PUBLISHER_ROOT/stream_space
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $PUBLISHER_ROOT/search_space

  # Etc
  chmod 755 /etc/opt/google/
  chmod 755 $BASEINSTALLDIR_OPT/etc/
  chmod 755 $BININSTALLROOTDIR/gevars.sh
  chmod 755 $BININSTALLROOTDIR/geserver

  # Run
  chmod 775 $BASEINSTALLDIR_OPT/run/
  chmod 775 $BASEINSTALLDIR_VAR/run/
  chown root:$GRPNAME $BASEINSTALLDIR_OPT/run/
  chown root:$GRPNAME $BASEINSTALLDIR_VAR/run/

  # Logs
  chmod 775 $BASEINSTALLDIR_VAR/log/
  chmod 775 $BASEINSTALLDIR_OPT/log/
  chown root:$GRPNAME $BASEINSTALLDIR_VAR/log
  chown root:$GRPNAME $BASEINSTALLDIR_OPT/log

  # Other folders
  chmod 755 $BASEINSTALLDIR_OPT
  chmod 755 $BASEINSTALLDIR_VAR
  chmod -R 755 /opt/google/lib/
  chmod -R 555 /opt/google/bin/
  chmod 755 /opt/google/bin
  chmod +s /opt/google/bin/gerestartapache /opt/google/bin/geresetpgdb /opt/google/bin/geserveradmin
  chmod -R 755 /opt/google/qt/
  # TODO: there is not such directory
  # chmod 755 /etc/opt/google/installed_products/
  chmod 755 /etc/opt/google/openldap/

  # Tutorial and Share
  find /opt/google/share -type d -exec chmod 755 {} \;
  find /opt/google/share -type f -exec chmod 644 {} \;
  chmod ugo+x /opt/google/share/searchexample/searchexample
  chmod ugo+x /opt/google/share/geplaces/geplaces
  chmod ugo+x /opt/google/share/support/geecheck/geecheck.pl
  chmod ugo+x /opt/google/share/support/geecheck/convert_to_kml.pl
  chmod ugo+x /opt/google/share/support/geecheck/find_terrain_pixel.pl
  chmod ugo+x /opt/google/share/support/geecheck/pg_check.pl
  # Note: this is installed in install_fusion.sh, but needs setting here too.
  chmod ugo+x $BASEINSTALLDIR_OPT/share/tutorials/fusion/download_tutorial.sh
  chown -R root:root /opt/google/share

  #TODO
  # Set context (needed for SELINUX support) for shared libs
  # chcon -t texrel_shlib_t /opt/google/lib/*so*
  # Restrict permissions to uninstaller and installer logs
  chmod -R go-rwx /opt/google/install

  # Disable cutter (default) during installation.
  /opt/google/bin/geserveradmin --disable_cutter

  # Restrict permissions to uninstaller and installer logs
  chmod -R go-rwx $INSTALL_LOG_DIR
}

show_final_success_message(){
 # Install complete
  INSTALLATION_OR_UPGRADE="installation"
  INSTALLED_OR_UPGRADED="installed"
  # TODO check for "UPGRADE" or case insensitive "upgrade"
  if  [[ "$SERVER_INSTALL_OR_UPGRADE" = *upgrade* ]]; then
    INSTALLATION_OR_UPGRADE="Upgrade"
    INSTALLED_OR_UPGRADED="upgraded"
  fi

          $BININSTALLROOTDIR/geserver start
          check_server_running;

  echo -e "Congratulations! $PRODUCT_NAME has been successfully $INSTALLED_OR_UPGRADED in the following directory:
          /opt/google \n
          Start Google Earth Enterprise Server with the following command:
          $BININSTALLROOTDIR/geserver start"

}

check_server_running()
{
  # Check that server is setup properly

  if ! check_server_processes_running; then
    echo -e "$GEES service is not running properly. Check the logs in
             /opt/google/gehttpd/logs \n
             /var/opt/google/pgsql/logs \n
             for more details. "
  fi
}

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------
main_postinstall
