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

#Create a package building sandbox directory. 
# This is typically ~/packages, but in corp environments where ~ is on an 
# NFS mount, the performance would be horrible. 
# In those cases you should place the dir on a local drive
#  (e.g. /localhome/<yourusername>/packages or even /packages). 
#prefix=/root
prefix=`pwd`
packages=$prefix/packages
mkdir -p $packages/BUILD $packages/RPMS $packages/SOURCES $packages/SPECS $packages/SRPMS
cat << EOF > ~/.rpmmacros
%_topdir $packages
%_tmppath /tmp
%__os_install_post \
%{nil}
EOF
cat << EOF > ~/.rpmrc
buildarchtranslate: athlon: i686
buildarchtranslate: i686: i686
buildarchtranslate: i586: i586
EOF

cat << EOF
Copy the spec files and source files into packages/SPECS and packages/SOURCES
then use:
 rpmbuild -bb blah.spec
 rpmbuild -bs blah.spec
EOF
