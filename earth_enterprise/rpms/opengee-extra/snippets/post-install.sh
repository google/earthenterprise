#!/bin/bash
#
# Copyright 2020 - 2021 The Open GEE Contributors
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

umask 002

main_postinstall()
{
    # Check if opengee-fusion and opengee-server are the expected version. If
    # not, we may be in the middle of an upgrade with incompatible libraries
    # and binaries. In that case, we skip the steps below and let
    # opengee-fusion and opengee-server take care of them.
    EXTRA_VERSION="${CURRENT_VERSION}"
    SERVER_VERSION=$(get_package_version opengee-server)
    FUSION_VERSION=$(get_package_version opengee-fusion)

    if [ "$SERVER_VERSION" == "$EXTRA_VERSION" ]; then
        install_searchexample_database

        # Set up the ExampleSearch plugin. The opengee-server-core RPM does
        # this by calling geresetpgdb, but there's no need to do a full reset
        # here.
        run_as_user "$GEPGUSER" "$BASEINSTALLDIR_OPT/bin/psql -q -d gesearch geuser -f $SQLDIR/examplesearch.sql"

        service geserver restart
    else
        echo "Skipping example search set up. It will be set up when opengee-server-${EXTRA_VERSION} is installed."
    fi

    if [ "$FUSION_VERSION" == "$EXTRA_VERSION" ]; then
        service gefusion stop
        add_fusion_tutorial_volume
        service gefusion start
    else
        echo "Skipping tutorial volume set up. It will be set up when opengee-fusion-${EXTRA_VERSION} is installed."
    fi
}

main_postinstall $@
