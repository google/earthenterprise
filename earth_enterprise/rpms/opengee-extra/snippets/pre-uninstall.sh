#!/bin/bash
#
# Copyright 2020 The Open GEE Contributors
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


main_preuninstall()
{
    # If the server is still installed then we need to remove the ExampleSearch
    # database entries. If the server has been uninstalled, the database is
    # already gone.
    if [ -f "/etc/init.d/geserver" ]; then
        echo "Deleting SearchExample Database"
        run_as_user "$GEPGUSER" "/opt/google/share/searchexample/searchexample delete"
        run_as_user "$GEPGUSER"  "$BASEINSTALLDIR_OPT/bin/psql -q -d gesearch geuser -f $SQLDIR/examplesearch_delete.sql"
        service geserver restart
    fi

    # If Fusion is still installed then we need to remove the tutorial source
    # volume. If fusion is no longer installed then there's nothing to do.
    if [ -f "/etc/init.d/gefusion" ]; then
        sudo service gefusion stop
        echo "Removing Tutorial Source Volume"
        "$BASEINSTALLDIR_OPT/bin/geconfigureassetroot" --noprompt \
            --assetroot "$ASSET_ROOT" --removevolume opt
        sudo service gefusion start
    fi
}

# On Red Hat this script could be called as part of an upgrade. The first
# argument to the script will be 0 if this is a full uninstall.
# On Debian systems the first parameter is "upgrade", "remove" or "purge".
# (See <https://wiki.debian.org/MaintainerScripts>.)
if [ "$1" = "0" ] || [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    main_preuninstall $@
fi
