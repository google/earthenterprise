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

add_fusion_tutorial_volume()
{
    # The tutorial directory can't be added as a source volume until
    # opengee-fusion-core and opengee-extra are both installed. Both RPMs call
    # this function, but only the second one will actually add the volume.

    CONFIG_ASSET_ROOT="${BASEINSTALLDIR_OPT}/bin/geconfigureassetroot"
    if [[ -d "${FUSION_TUTORIAL_DIR}" && -f "${CONFIG_ASSET_ROOT}" ]]; then
        RET_VAL=0
        "${CONFIG_ASSET_ROOT}" --addvolume \
            "opt:${FUSION_TUTORIAL_DIR}" --noprompt || RET_VAL=$?
        if [ "$RET_VAL" -eq "255" ]; then
            cat <<END
The geconfigureassetroot utility has failed while attempting
to add the volume 'opt:${FUSION_TUTORIAL_DIR}'.
This is probably because a volume named 'opt' already exists.
END
        fi
    fi
}
