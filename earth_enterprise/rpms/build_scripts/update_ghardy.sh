#!/bin/bash
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

# This script updates ghardy boxes to add the gcc 4.1.1 compatiblity libs.
# Usage: update_ghardy.sh

# Require root.
if [ `whoami` != root ]
then
    echo ---------------
    echo No Root Permission
    echo ---------------
    echo
    echo You must have root privileges to do this update.
    echo Log in as the root user or use the "sudo" command to run this installer.
    echo
    exit -1
fi

# Unpack the compatibility libs and move them under
# /opt/google/lib64
tar xzf ghardy-compatibility-libs.tar.gz
mv compatibility /opt/google/lib64
cd /opt/google/lib64/compatibility

# Create the necessary symlinks.
ln -f -s ld-2.3.3.so ld-linux-x86-64.so.2
ln -f -s libICE.so.6.3 libICE.so.6
ln -f -s libSM.so.6.0 libSM.so.6
ln -f -s libX11.so.6.2 libX11.so.6
ln -f -s libXcursor.so.1.0.2 libXcursor.so.1
ln -f -s libXext.so.6.4 libXext.so.6
ln -f -s libXft.so.2.1.1 libXft.so.2
ln -f -s libXinerama.so.1.0 libXinerama.so.1
ln -f -s libXmu.so.6.2 libXmu.so.6
ln -f -s libXrandr.so.2.0 libXrandr.so.2
ln -f -s libXrender.so.1.2.2 libXrender.so.1
ln -f -s libXt.so.6.0 libXt.so.6
ln -f -s libfontconfig.so.1.0.4 libfontconfig.so.1
ln -f -s libfreetype.so.6.3.5 libfreetype.so.6
ln -f -s libjpeg.so.62.0.0 libjpeg.so.62
ln -f -s libjpeg.so.62.0.0 libjpeg.so
ln -f -s liblcms.so.1.0.12 liblcms.so.1
ln -f -s libpng12.so.0.1.2.5 libpng12.so.0
ln -f -s libpng12.so.0 libpng12.so
ln -f -s libsasl2.so.2.0.18 libsasl2.so.2
ln -f -s libz.so.1 libgz.so.1
ln -f -s libz.so.1 libz.so


