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

    check_username "$GEAPACHEUSER"
    check_username "$GEPGUSER"

    # Dump database if it exists
    database_backup

    # If user previously symlinked to another location for portable globes storage don't let the installer mess that up
    save_portable_globe_symlink

    # Preserve Admin Console password if it exists. To be restored on post-install
    if [ -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/.htpasswd" ]; then
        mv -f "$BASEINSTALLDIR_OPT/gehttpd/conf.d/.htpasswd" "/tmp/"
    fi
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

check_username()
{
    USERNAME_EXISTS=$(getent passwd "$1")

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p "$BASEINSTALLDIR_OPT/.users/$1" || return 1
        useradd --home "$BASEINSTALLDIR_OPT/.users/$1" --system --gid "$GEGROUP" "$1"
    else
        # user already exists -- update primary group
        usermod -g "$GEGROUP" "$1"
    fi
    if [ ! -d "$BASEINSTALLDIR_OPT/.users/$1" ]; then
        mkdir -p "$BASEINSTALLDIR_OPT/.users/$1"
    fi
    chown -R "$1:$GEGROUP" "$BASEINSTALLDIR_OPT/.users/$1"

}

database_backup()
{
    # If the GEE data directory exists and PostgreSQL is installed
    if [ -d "$BASEINSTALLDIR_VAR/pgsql/data" ] && [ -f "$BASEINSTALLDIR_OPT/bin/psql" ]; then
        do_dump
    fi
}

save_portable_globe_symlink()
{
    # Move a symlink for portables to save it from the RPM extract changing ownership to root:root
    # It will be moved back as part of the post install script
    if [ -L "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes" ]; then
        mv "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes" "$BASEINSTALLDIR_OPT/gehttpd/htdocs/cutter/globes_symlink"
    fi
}

#-----------------------------------------------------------------
# Pre-install Main
#-----------------------------------------------------------------

main_preinstall $@
