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
#-----------------------------------------------------------------

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_postuninstall()
{
    # if we really get error, something really broken elsewhere
    userdel $GEAPACHEUSER
    userdel $GEPGUSER
}


#-----------------------------------------------------------------
# Post-Uninstall Main
#-----------------------------------------------------------------

main_postuninstall "$@"
