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

install_searchexample_database()
{
    # The search example database can only be installed after opengee-server-core
    # (which provides postgresql) and opengee-extra (which provides searchexample)
    # are both installed. Both RPMs call this function and the second one will
    # run the install.

    if [[ -f "${PGSQL_PROGRAM}" && -f "${SEARCH_EX_SCRIPT}" ]]; then
        echo "Installing SearchExample Database"
        run_as_user "$GEPGUSER" "/opt/google/share/searchexample/searchexample create"
    fi
}
