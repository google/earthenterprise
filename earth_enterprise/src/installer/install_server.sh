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

# script arguments

# config values
PUBLISHER_ROOT="/usr/local/google/gevol/publish_dbs"
#SOURCE_VOLUME="/usr/local/google/gevol/src"
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
KH_SYSTEMRC="/usr/keyhole/etc/systemrc"
INITSCRIPTUPDATE="/usr/sbin/update-rc.d"
CHKCONFIG="/sbin/chkconfig"
OS_RELEASE="/etc/os-release"

# derived directories
BASEINSTALLGDALSHAREDIR="$BASEINSTALLDIR_OPT/share/gdal"
PATH_TO_LOGS="$BASEINSTALLDIR_VAR/log"
SYSTEMRC="$BASEINSTALLDIR_ETC/systemrc"
OLD_SYSTEMRC=$SYSTEMRC
SOURCECODEDIR=$(dirname $(dirname $(readlink -f "$0")))
TOPSOURCEDIR_EE=$(dirname $SOURCECODEDIR)
GESERVERBININSTALL="$BININSTALLROOTDIR/gesever"
INSTALL_LOG_DIR="$BASEINSTALLDIR_OPT/install"
INSTALL_LOG="$INSTALL_LOG_DIR/geserver_install_$(date +%Y_%m_%d.%H%M%S).log"

# versions and user names
GEES="Google Earth Enterprise Server"
SHORT_VERSION="5.1"
LONG_VERSION="5.1.3"
GRPNAME="gegroup"
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"

#TODO this value could come from the server install script - merge as required (dattam)
SERVER_UNINSTALL_OR_UPGRADE="uninstall"
GEE_CHECK_CONFIG_SCRIPT="/opt/google/gehttpd/cgi-bin/set_geecheck_config.py"
PGSQL_DATA="/var/opt/google/pgsql/data"
PGSQL_LOGS="/var/opt/google/pgsql/logs"
PGSQL_PROGRAM="/opt/google/bin/pg_ctl"
PRODUCT_NAME="Google Earth Enterprise"
CHECK_POST_MASTER=""
CHECK_GEHTTPD=""
START_SERVER_DAEMON=0
PROMPT_FOR_START="n"

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
  $BASEINSTALLDIR_OPT/bin/geconfigurepublishroot --noprompt --path=$PUBLISHER_ROOT$

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

postgres_db_config()
{
  # a) PostGres folders and configuration
  chmod -R 700 /var/opt/google/pgsql/
  chown -R $GEPGUSER_NAME:$GRPNAME /var/opt/google/pgsql/
  reset_pgdb
}

run_as_user() {
 use_su="su $1 -c 'echo -n 1' 2> /dev/null  || echo -n 0"
 if [ "$use_su" -eq 1 ] ; then
    ( cd / ;su $1 -c "$2" )
 else
    ( cd / ;sudo -u $1 $2 )
 fi
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
   echo "Setting up the geserver daemon...\n"
   test -f $CHKCONFIG && $CHKCONFIG --add gefusion
   test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE -f gefusion remove
   test -f $INITSCRIPTUPDATE && $INITSCRIPTUPDATE gefusion start 90 2 3 4 5 . stop 10 0 1 6 .
   printf "Fusion daemon setup ... DONE\n"
}
  
# 6) Install the GEPlaces and SearchExample Databases
install_search_databases()
{
  # a) Start the PSQL Server
  echo "   # a) Start the PSQL Server "
  run_as_user $GEPGUSER_NAME $PGSQL_PROGRAM -D $PGSQL_DATA -l $PGSQL_LOGS/pg.log start -w

  echo "  # b) Install GEPlaces Database"
  # b) Install GEPlaces Database
  run_as_user $GEPGUSER_NAME "/opt/google/share/geplaces/geplaces create"

  echo  "  # c) Install SearchExample Database "
  # c) Install SearchExample Database
  run_as_user $GEPGUSER_NAME "/opt/google/share/searchexample/searchexample create"

  # d) Stop the PSQL Server
  echo  "stopping the pgsql instance"
  run_as_user $GEPGUSER_NAME $PGSQL_PROGRAM -D $PGSQL_DATA stop
}

modify_files()
{
  # Replace IA users in a file if they are found.
  # this step not requied for the OSS installs, however
  # if there are non-OSS installs in the system, it might help cleanup those.

  # a) Search and replace the below variables in /etc/init.d/geserver.
  sed -i 's/IA_GEAPACHE_USER/GEAPACHEUSER_NAME/' $BININSTALLROOTDIR/geserver
  sed -i 's/IA_GEPGUSER/GEPGUSER_NAME/' $BININSTALLROOTDIR/geserver

  # b) Search and replace the file ‘/opt/google/gehttpd/conf/gehttpd.conf’
  sed -i 's/IA_GEAPACHE_USER/GEAPACHEUSER_NAME/' /opt/google/gehttpd/conf/gehttpd.conf
  sed -i 's/IA_GEPGUSER/GEPGUSER_NAME/' /opt/google/gehttpd/conf/gehttpd.conf

  # c) Create a new file ‘/etc/opt/google/fusion_Server_version’ and
  # add the below text to it.
  echo $LONG_VERSION > /etc/opt/google/fusion_Server_version

  # d) Create a new file ‘/etc/init.d/gevars.sh’ and prepend the below lines.
  echo -e 'GEAPACHEUSER=GEAPACHEUSER_NAME \n 
        GEPGUSER=GEPGUSER_NAME \n
        GEFUSIONUSER=GEFUSIONUSER_NAME \n
        GEGROUP=GRPNAME' > $BININSTALLROOTDIR/gevars.sh
}

fix_postinstall_filepermissions()
{
  # PostGres
  chown -R $GEPGUSER_NAME:$GRPNAME $BASEINSTALLDIR_VAR/pgsql/

  # Apache
  mkdir -p $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime
  chmod -R 755 $BASEINSTALLDIR_OPT/gehttpd
  chmod -R 775 $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime/
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime/
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/conf.d/virtual_servers/runtime/
  chown -R $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes/
  chmod -R 700 $BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes/
  chown $GEAPACHEUSER_NAME:$GRPNAME $BASEINSTALLDIR_OPT/gehttpd/htdocs/.htaccess

  # Publish Root
  chmod 775 $PUBLISHER_ROOT/stream_space
  # TODO - Not Found
  # chmod 644 $PUBLISHER_ROOT/stream_space/.config
  chmod 644 $PUBLISHER_ROOT/.config
  chmod 755 $PUBLISHER_ROOT
  chown $GEAPACHEUSER_NAME:$GRPNAME $PUBLISHER_ROOT/stream_space
  chown $GEAPACHEUSER_NAME:$GRPNAME $PUBLISHER_ROOT/search_space

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
  chmod 775 /var/$BASEINSTALLDIR_OPT/log/
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
  if  [[ "$SERVER_UNINSTALL_OR_UPGRADE" = *upgrade* ]]; then
    INSTALLATION_OR_UPGRADE="Upgrade"
    INSTALLED_OR_UPGRADED="upgraded"
  fi

  # Ask the user to start the GEES service
  if [ "$START_SERVER_DAEMON" == 1 ]; then
    while true; do
      echo -e "Do you want to start the $GEES Service(y/n)?"
      read PROMPT_FOR_START
      case $PROMPT_FOR_START in
        [Yy]* )
          $BININSTALLROOTDIR/geserver start
          check_server_running; break;;
        [Nn]* )
          echo -e "Congratulations! $PRODUCT_NAME has been successfully $INSTALLED_OR_UPGRADED in the following directory:
                   /opt/google \n
                   Start Google Earth Enterprise Server with the following command:
                   $BININSTALLROOTDIR/geserver start"
          exit;;
        * ) echo "Please answer yes or no.";;
    esac
    done
  else
    echo -e  " $GEES could not start properly."
  fi

  echo -e "Congratulations! $PRODUCT_NAME has been successfully $INSTALLED_OR_UPGRADED in the following directory:
          /opt/google \n
          Start Google Earth Enterprise Server with the following command:
          $BININSTALLROOTDIR/geserver start"
  
}

check_server_running()
{
  # Check that server is setup properly
  # i) Check postgres is running .Store o/p in CHECK_POST_MASTER
  CHECK_POST_MASTER=$( ps -e | grep postgres)
  # ii) Check gehttpd is running  .Store the o/p in CHECK_GEHTTPD
  CHECK_GEHTTPD=$( ps -e| grep gehttpd )
  if [ -z "$CHECK_POST_MASTER" ] || [ -z "$CHECK_GEHTTPD" ]; then
    echo -e "$GEES service is not running properly. Check the logs in 
             /opt/google/gehttpd/logs \n 
             /var/opt/google/pgsql/logs \n 
             for more details. "
  fi
}

#-----------------------------------------------------------------
# Post-Install Main
#-----------------------------------------------------------------
mkdir -p $INSTALL_LOG_DIR
exec 2> $INSTALL_LOG

main_postinstall
