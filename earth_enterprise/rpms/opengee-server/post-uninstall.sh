#! /bin/bash
#
# Copyright 2017 The Open GEE Contributors
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
# Main Functions
#-----------------------------------------------------------------
main_postuninstall()
{
    # Fedora recommends to never delete users that a package has created:
    #   https://fedoraproject.org/wiki/Packaging:UsersAndGroups
    # TODO: Consider whether we want to remove this:
    if getent passwd "$GEAPACHEUSER" >/dev/null; then
        userdel "$GEAPACHEUSER"
    fi
    if getent passwd "$GEPGUSER" >/dev/null; then
        userdel "$GEPGUSER"
    fi
}


#-----------------------------------------------------------------
# Post-Uninstall Main
#-----------------------------------------------------------------

main_postuninstall $@
