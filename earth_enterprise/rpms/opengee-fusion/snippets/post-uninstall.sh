#!/bin/bash
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


remove_users_and_groups()
{
    echo "OpenGEE user $GEFUSIONUSER may be removed once associated data files are purged."
}


#-----------------------------------------------------------------
# Main Function
#-----------------------------------------------------------------

# On Red Hat the first argument to install scripts is "the number of versions
# of the package that are installed".  It is 0 when uninstalling the last
# version, 1 on first install, 2 or higher on upgrade.
#     On Debian systems the first parameter is "upgrade", "remove" or "purge".
# (See <https://wiki.debian.org/MaintainerScripts>.)
if [ "$1" = "0" ] || [ "$1" = "purge" ] ; then
    remove_users_and_groups
    if [ `command -v systemctl` ]; then systemctl daemon-reexec; fi
fi
