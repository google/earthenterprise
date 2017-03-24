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

######(deprecated, we dont use svn anymore)#########

The Fusion Build Process consists of several steps.
The "trunk/rpms" folder contains the stuff necessary for step 0 of the build process
bootstrap an RPM-based build machine (DEB coming later).

Note: to support RedHat4 and SuSE9 and up, 
      you should build with one of those 2 or CentOS4!


Here's what to do to bootstrap and build Fusion on a single architecture:

# 1: install subversion (it seems 1.1 is sufficient to download the fusion source)
yum install subversion -y

# 2: download the Fusion source trunk
mkdir ~/tree
cd ~/tree
svn co 'svn://hostname/r/legacy/gefusion/trunk/' -q

# 3: Install the Build Tools
cd ~/tree/trunk/rpms
./install_build_tools.pl

# 4: a) if building the RPMS from scratch
#       Build the developer RPMs (also installs them)
export PATH=/opt/google/bin:$PATH
scons -j4

# 4: b) use the existing Golden RPMS, they need to be installed
./install_third_party_rpms.pl

# 5: Build the Fusion RPM packages (will build the source in release mode)
# Make the RPMS
cd ~/tree/trunk/src/rpm
make all
cd ~/tree/trunk/searchexample
make
cd ~/tree/trunk/geplaces
make
cd ~/tree/trunk/tutorial
make

# 6: Creating the installer package
# Note: generally this is done by pulling RPMS from an i686 and x86_64 build machine
# but to make a single package (e.g., "pro") on the current architecture use:
cd ~/tree/trunk/src/packager
./MakePackages.pl --packagedir ~/packages --local ~/tree/trunk/rpms/packages --overwrite --package pro

