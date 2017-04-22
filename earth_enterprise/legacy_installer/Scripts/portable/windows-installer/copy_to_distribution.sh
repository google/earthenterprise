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

# This Script will copy the portable window installer exe files from
# user temp directory /home/user/share to distribution directory on
# master machine (fusionbuilder) where "user" is taken
# from commandline.
#
# Usage: /usr/local/google/pulse2/data/agents/master/work/
# 02Fusion_Trunk_Installer/googleclient/geo/earth_enterprise/installer/
# Scripts/portable/windows-installer/copy_to_distribution.sh songxu
#

echo "copy_to_distribution.sh"
echo 'pwd'
pwd

temp_dir=/home/$1/share
distribution_dir=/usr/local/google/gepackager/dist

cp -f $temp_dir/GEEPortableWindowsInstaller-* $distribution_dir
rm $temp_dir/GEEPortableWindowsInstaller-*
