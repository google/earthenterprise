#!/bin/bash
#
#
# Copyright 2017 Google Inc.
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

installer_dir="$1"  # e.g /tmp/fusion_trunk_install
exit_status=0
for target in fusion server
do
  for bin in $installer_dir/$target/opt/google/bin/*
  do
    for lib in `ldd $bin 2>/dev/null | grep 'REL-x86_64' | cut -f2 -d'>' | cut -f 2 -d' ' | xargs -n1 -ixxx basename xxx`
    do
      lib_check_list="$installer_dir/$target/opt/google/lib $installer_dir/common/opt/google/lib $installer_dir/common/opt/google/qt/lib";
      if [ -d $installer_dir/common/opt/google/lib64 ]; then
        lib_check_list="$lib_check_list $installer_dir/common/opt/google/lib64";
      fi
      fname=`find $lib_check_list -name "$lib" -exec bash -c "test -r {} && echo {}" \;`
      if [ "$fname" == "" ]; then
        echo "Cannot find $lib for $target $bin"
        exit_status=1
      fi
    done
  done
done
exit $exit_status
