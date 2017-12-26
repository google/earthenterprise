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
umask 002

# get script directory
SCRIPTDIR=`dirname $0`

. $SCRIPTDIR/common.sh

# config values
PUBLISHER_ROOT="/gevol/published_dbs"
INSTALL_LOG="$INSTALL_LOG_DIR/geserver_install_$(date +%Y_%m_%d.%H%M%S).log"
BACKUP_DIR="$BASEINSTALLDIR_VAR/server-backups/$(date +%Y_%m_%d.%H%M%S)"

# script arguments
BACKUPSERVER=true
BADHOSTNAMEOVERRIDE=false
MISMATCHHOSTNAMEOVERRIDE=false
DEFAULTGROUPNAME="gegroup"
GROUPNAME=$DEFAULTGROUPNAME

# user names
GRPNAME="gegroup"
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME

SERVER_INSTALL_OR_UPGRADE="install"
GEE_CHECK_CONFIG_SCRIPT="/opt/google/gehttpd/cgi-bin/set_geecheck_config.py"
PGSQL_DATA="/var/opt/google/pgsql/data"
PGSQL_LOGS="/var/opt/google/pgsql/logs"
PGSQL_PROGRAM="/opt/google/bin/pg_ctl"
PRODUCT_NAME="$GEE"
START_SERVER_DAEMON=1
PROMPT_FOR_START="n"

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
  # 3) Introduction
  show_intro

  # 2) Root/Sudo check
  if [ "$EUID" != "0" ]; then
    show_need_root
    exit 1
  fi

  # Argument check
  if ! parse_arguments "$@"; then
    exit 1
  fi

  if ! determine_os; then
    exit 1
  fi

  # 7) Check prerequisite software
  if ! check_prereq_software; then
    exit 1
  fi

  # check to see if GE Server processes are running
  if check_server_processes_running; then
    show_server_running_message
    exit 1
  fi

  # tmp dir check
  if [ ! -d "$TMPINSTALLDIR" ]; then
    show_no_tmp_dir_message $TMPINSTALLDIR
    exit 1
  fi

  # 5b) Perform backup
  if [ $BACKUPSERVER == true ]; then
    # Backing up current Server Install...
    backup_server
  fi

  # 6) Check valid host properties
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

  # 8) Check if group and users exist
  check_group

  check_username $GEAPACHEUSER_NAME
  check_username $GEPGUSER_NAME

  # 10) Configure Publish Root
  configure_publish_root
}

check_prereq_software()
{
  local check_prereq_software_retval=0
  local script_name="$GEES $LONG_VERSION installer"

  if ! software_check "$script_name" "libxml2-utils" "libxml2.*x86_64"; then
    check_prereq_software_retval=1
  fi

  if ! software_check "$script_name" "python2.[67]" "python-2.[67].*"; then
    check_prereq_software_retval=1
  fi

  return $check_prereq_software_retval
}

main_install()
{
  copy_files_to_target
  create_links
}

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

show_intro()
{
  echo -e "\nWelcome to the $GEES $LONG_VERSION installer."
}

# TODO: Add additional parameters for GEE Server
show_help()
{
  echo -e "\nUsage: \tsudo ./install_server.sh [-h] [-dir /tmp/fusion_os_install -hnf -hnmf]\n"

  echo -e "-h --help \tHelp - display this help screen"
  echo -e "-dir \t\tTemp Install Directory - specify the temporary install directory. Default is [$TMPINSTALLDIR]."
  echo -e "-hnf \t\tHostname Force - force the installer to continue installing with a bad \n\t\thostname. Bad hostname values are [${BADHOSTNAMELIST[*]}]."
  echo -e "-hnmf \t\tHostname Mismatch Force - force the installer to continue installing with a \n\t\tmismatched hostname.\n"
}

# TODO convert to common function
show_need_root()
{
  echo -e "\nYou must have root privileges to install $GEES.\n"
  echo -e "Log in as the $ROOT_USERNAME user or use the 'sudo' command to run this installer."
  echo -e "The installer must exit."
}

# TODO Add additional parameters for server
parse_arguments()
{
  local parse_arguments_retval=0

  while [ $# -gt 0 ]
  do
    case "${1,,}" in
      -h|-help|--help)
        show_help
        parse_arguments_retval=1
        break
        ;;
      -hnf)
        BADHOSTNAMEOVERRIDE=true;
        ;;
      -hnmf)
        MISMATCHHOSTNAMEOVERRIDE=true
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

  return $parse_arguments_retval;
}

show_server_running_message()
{
  echo -e "\n$GEES has active running processes."
  echo -e "To use this installer to upgrade, you must stop all geserver services.\n"
}

configure_publish_root()
{
  # Update PUBLISHER_ROOT if geserver already installed
  local STREAM_SPACE="$GEHTTPD_CONF/stream_space"
  if [ -e $STREAM_SPACE ]; then
    PUBLISHER_ROOT=`cat $STREAM_SPACE |cut -d" " -f3 |sed 's/.\{13\}$//'`
  fi
}

#-----------------------------------------------------------------
# Install Functions
#-----------------------------------------------------------------
copy_files_to_target()
{
  printf "\nCopying files from source to target directories..."

  mkdir -p $BASEINSTALLDIR_OPT/share/doc
  mkdir -p $BASEINSTALLDIR_OPT/gehttpd/conf
  mkdir -p $BASEINSTALLDIR_OPT/gehttpd/htdocs/shared_assets/images
  mkdir -p $BASEINSTALLDIR_OPT/search
  mkdir -p $BASEINSTALLDIR_VAR/openssl/private
  mkdir -p $BASEINSTALLDIR_VAR/openssl/misc
  mkdir -p $BASEINSTALLDIR_VAR/openssl/certs
  mkdir -p $BASEINSTALLDIR_ETC/openldap
  mkdir -p $BASEINSTALLDIR_VAR/pgsql

  local error_on_copy=0
  cp -rf $TMPINSTALLDIR/common/opt/google/bin $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/common/opt/google/share $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/common/opt/google/qt $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/common/opt/google/qt/lib $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/common/opt/google/lib $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/common/opt/google/gepython $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/share $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/bin $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/lib $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/gehttpd $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/search $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/searchexample/opt/google/share $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/geplaces/opt/google/share $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/AppacheSupport/opt/google/gehttpd $BASEINSTALLDIR_OPT
  if [ $? -ne 0 ]; then error_on_copy=1; fi

  cp -rf $TMPINSTALLDIR/server/etc/init.d/geserver $BININSTALLROOTDIR
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/etc/profile.d/ge-server.sh $BININSTALLPROFILEDIR
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/etc/profile.d/ge-server.csh $BININSTALLPROFILEDIR
  if [ $? -ne 0 ]; then error_on_copy=1; fi

  cp -rf $TMPINSTALLDIR/server/user_magic/etc/logrotate.d/gehttpd $BASEINSTALLLOGROTATEDIR
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/user_magic/var/opt/google/pgsql/logs/ $BASEINSTALLDIR_VAR/pgsql
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/manual/opt/google/share/doc/manual $BASEINSTALLDIR_OPT/share/doc
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/gehttpd/conf/gehttpd.conf $BASEINSTALLDIR_OPT/gehttpd/conf
  if [ $? -ne 0 ]; then error_on_copy=1; fi
  cp -rf $TMPINSTALLDIR/server/opt/google/gehttpd/htdocs/shared_assets/images/location_pin.png $BASEINSTALLDIR_OPT/gehttpd/htdocs/shared_assets/images
  if [ $? -ne 0 ]; then error_on_copy=1; fi

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

  # TODO: final step: copy uninstall script
  # cp -f $TMPOPENLDAPPATH/<........> $INSTALL_LOG_DIR

  if [ $error_on_copy -ne 0 ]
  then
    show_corrupt_tmp_dir_message $TMPINSTALLDIR $INSTALL_LOG
    exit 1
  fi

  printf "DONE\n"
}

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
  INSTALLED_OR_UPGRADED="installed"
  # TODO check for "UPGRADE" or case insensitive "upgrade"
  if  [[ "$SERVER_INSTALL_OR_UPGRADE" = *upgrade* ]]; then
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

  if ! check_server_processes_running; then
    echo -e "$GEES service is not running properly. Check the logs in
             /opt/google/gehttpd/logs \n
             /var/opt/google/pgsql/logs \n
             for more details. "
  fi
}

#-----------------------------------------------------------------
# Pre-Install Main
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
