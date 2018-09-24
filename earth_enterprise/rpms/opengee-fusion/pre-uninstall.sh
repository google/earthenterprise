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
#
# OSPackage hard-codes the interpreter to `/bin/sh` for Debian systems, so make
# sure this scripts runs in `sh`, even if you specify `bash` above.

set +x
set -e

#-----------------------------------------------------------------
# Main Function
#-----------------------------------------------------------------

# remove if actually uninstalling
if [ "$1" = "0" ] ; then
	service gefusion stop
	remove_service gefusion
fi

#-----------------------------------------------------------------
