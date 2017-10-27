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

#-----------------------------------------------------------------
# Post-install Main
#-----------------------------------------------------------------

main_postinstall "$@"
