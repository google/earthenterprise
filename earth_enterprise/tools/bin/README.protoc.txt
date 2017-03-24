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

protoc is the Google Protocol Buffer compiler.
For Google Earth Enterprise Fusion, we build this on a 64 bit SLES box

The build process involves:
# tar'ing up the protobuf source:
cd /home/build/opensource/
tar cfz /tmp/protobuf.tar.gz protobuf

# SSH into the build machine and copy the protobuf tarball over.
ssh khbuild@seb
# Assume root privileges (may not be completely necessary)
sudo su -
scp username@hostname:/tmp/protobuf.tar.gz .
tar xzf protobuf.tar.gz

cd protobuf
# Add /opt/google/bin to path to pick up the latest g++/gcc
export PATH=/opt/google/bin:$PATH
# Configure to use static libs
./autogen.sh
./configure --disable-shared
make


# Copy the executable and the config header into place
scp src/protoc config.h username@hostname:/tmp
# clean up the remnants
make clean
cd ..
rm -rf protobuf*

# On your code editing machine
g4 edit googleclient/geo/earth_enterprise/tools/bin
mv /tmp/protoc googleclient/geo/earth_enterprise/tools/bin

g4 edit googleclient/geo/earth_enterprise/src/google/protobuf/config.h # jgd config.h was client.h
mv /tmp/config.h googleclient/geo/earth_enterprise/src/google/protobuf
