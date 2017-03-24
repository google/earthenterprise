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

set -e

PACKAGE_DIR="/tmp/gee_install"

do_build=0
case $1 in
  -build|--build) do_build=1;;
esac

sudo /etc/init.d/gefusion stop
sudo /etc/init.d/geserver stop

# Clean up after installing GEE.
# Note: need this cleanup before building.
if [ ! -d /opt/google/_bin ]; then
  sudo mv /opt/google/bin /opt/google/_bin
fi

if [ ! -d /opt/google/_lib ]; then
  sudo mv /opt/google/lib /opt/google/_lib
fi

if [ ! -d /opt/google/_lib64 ]; then
  sudo mv /opt/google/lib64 /opt/google/_lib64
fi

if [ ! -d /opt/google/_gepython ] ; then
  sudo mv /opt/google/gepython /opt/google/_gepython
fi


if [ $do_build -eq 1 ]; then
  # build
  scons -j8 release=1 third_party
  scons -j8 release=1
  # deploy files for packaging.
  scons -j8 release=1 installdir=$PACKAGE_DIR install
  #cd ..
  #scons -j8  installdir=$PACKAGE_DIR install
  #cd -
fi


# Install: deploy to /opt/google.
deploy_dirs=$(find $PACKAGE_DIR -mindepth 1 -maxdepth 1 -type d)
for deploy_dir in $deploy_dirs
do
  # TODO: handle subdirs 'AppacheSupport' and 'user_magic' in install_dir/server directory.
  find $deploy_dir -mindepth 1 -maxdepth 1 -type d -exec sudo rsync -rltpv "{}" / \;
done

sudo chmod 755 /etc/init.d/geserver
sudo chmod 755 /etc/init.d/gefusion

# altering GEE Server
sudo patch /opt/google/gehttpd/conf/gehttpd.conf ./tmp/gehttpd_deploy.patch
sudo patch /etc/init.d/geserver ./tmp/geserver_deploy.patch

sudo chmod -R 755 /opt/google/gehttpd

# altering GEE Fusion
sudo chmod 755 /opt/google/share/site-icons/*
sudo chmod 755 /opt/google/share/fonts/fontlist
sudo chmod 755 /opt/google/share/fonts/luxisr.ttf

# start daemons
sudo /etc/init.d/gefusion start
sudo /etc/init.d/geserver start

# for python bindings
#export PYTHONPATH=/opt/google/gepython/Python-2.7.6/lib/python2.7/site-packages/:$PYTHONPATH

