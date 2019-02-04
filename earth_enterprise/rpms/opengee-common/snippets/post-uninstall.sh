#!/bin/bash
#
# Copyright 2018 the Open GEE Contributors
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


#------------------------------------------------------------------------------
# Directory locations:
BININSTALLROOTDIR="/etc/init.d"

#------------------------------------------------------------------------------
# Group names:

remove_users_groups()
{
    echo "OpenGEE group $GEGROUP may be removed once associated data files are purged."
}

#-----------------------------------------------------------------
# Main Function:
#-----------------------------------------------------------------

# * In Red Hat $1 = 0 indicates un-install, and not an upgrade.
# * On Debian $1 = "purge" happens only during an uninstall when the user
# requests no configuration be left behind.
if [ "$1" = "0" ] || [ "$1" = "purge" ]; then
    remove_users_groups
fi
#-----------------------------------------------------------------
