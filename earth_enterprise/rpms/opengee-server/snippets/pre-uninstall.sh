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


#-----------------------------------------------------------------
# Main Function
#-----------------------------------------------------------------

# Remove if actually uninstalling.
#
# On Red Hat the first argument to install scripts is "the number of versions
# of the package that are installed".  It is 0 when uninstalling the last
# version, 1 on first install, 2 or higher on upgrade.
#     On Debian systems the first parameter is "upgrade", or "remove".
# (See <https://wiki.debian.org/MaintainerScripts>.)
case "$1" in
	0|remove|upgrade)
		service geserver stop || exit 1
		remove_service geserver
		;;
esac
