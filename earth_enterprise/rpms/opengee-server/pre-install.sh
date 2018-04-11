#! /bin/bash
#
# Copyright 2018 The Open GEE Contributors
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
set -e

NEW_INSTALL=false
if [ "$1" = "1" ] ; then
    NEW_INSTALL=true
fi

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
     # Report errors to RPM installer
    set -e

    # Check to see if opengee executables work and error out if not
    RET_VAL=0
    ERROUT=`$BASEINSTALLDIR_OPT/bin/geserveradmin 2>&1` || RET_VAL=$?

    if [ "$RET_VAL" -eq "127" ]; then
      echo "$ERROUT"
      echo "It appears that not all library dependencies have been installed."
      echo "This is likely to be a missing MrSID library."
      return 127
    fi

    # Stop reporting errors to RPM installer
    set +e

    # needed both for rpm state change and upgrading non-rpm install
    if [ -f /etc/init.d/geserver ]; then
        service geserver stop
    fi

    # only if a new install, has failsafe to protect non-rpm upgrades
    if [ "$NEW_INSTALL" = "true" ] ; then
        check_username "$GEAPACHEUSER"
        check_username "$GEPGUSER"

    fi

    # Dump database if it exists
    database_backup

}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_username()
{
    USERNAME_EXISTS=$(getent passwd "$1")

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p "$BASEINSTALLDIR_OPT/.users/$1"
        useradd --home "$BASEINSTALLDIR_OPT/.users/$1" --system --gid "$GEGROUP" "$1"
    else
        # user already exists -- update primary group
        usermod -g "$GEGROUP" "$1"
    fi

}

database_backup()
{
    # If the GEE data directory exists and PostgreSQL is installed
    if [ -d "$BASEINSTALLDIR_VAR/pgsql/data" ] && [ -f "$BASEINSTALLDIR_OPT/bin/psql" ]; then
        do_dump
    fi
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall $@
