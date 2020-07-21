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

umask 002

main_postinstall()
{
    if [ -f "/etc/init.d/geserver" ]; then
        install_searchexample_database

        # Set up the ExampleSearch plugin. The opengee-server-core RPM does
        # this by calling geresetpgdb, but there's no need to do a full reset
        # here.
       "$BASEINSTALLDIR_OPT/bin/psql" -q -d gesearch geuser -f "$SQLDIR/examplesearch.sql"

        service geserver restart
    fi

    if [ -f "/etc/init.d/gefusion" ]; then
        service gefusion stop
        add_fusion_tutorial_volume
        service gefusion start
    fi
}

main_postinstall $@
