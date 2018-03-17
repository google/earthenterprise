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

#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_postuninstall()
{
    echo "OpenGEE users $GEAPACHEUSER and $GEPGUSER may be removed once associated data files are purged."
}


#-----------------------------------------------------------------
# Post-Uninstall Main
#-----------------------------------------------------------------

if [ "$1" = "0" ] ; then
    main_postuninstall $@
fi
