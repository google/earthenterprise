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


set -e                 # Exit at 1st error

usage_print=0
exit_status=0
case $1 in
  -h|-help|--help|--usage) usage_print=1;;
  *) case $# in
       5) true;;
       *) usage_print=1; exit_status=1;;
     esac
esac

case $usage_print in
  1)
  cmd=`basename $0`
  echo "Usage   : $cmd gepython_dir server_dir globe_dir tornado_tar_ball output_tar_ball"
  echo "Example : $cmd /tmp/fusion_trunk_install/common/opt/google/gepython src/fusion/portableglobe/servers src/fusion/portableglobe/globes third_party/tornado/tornado-0.2.tar.gz ~/linux_installer"
  echo "          Compiles python files and copies those to installer dir."
  echo "          Copies /opt/google/gepython to installer dir."
  echo "          Builds tornado into this python installation."
  echo "          Removes unwanted files and packs all as a self installing bash script."
  echo ""
  exit $exit_status
  ;;
esac

# The following readlink technique is to convert a relative file name to
# absolute file name.
gepython_dir="$( readlink -f "$( dirname "$1" )" )/$( basename "$1" )"
server_dir="$( readlink -f "$( dirname "$2" )" )/$( basename "$2" )"
globe_dir="$( readlink -f "$( dirname "$3" )" )/$( basename "$3" )"
tornado_tar_ball="$( readlink -f "$( dirname "$4" )" )/$( basename "$4" )"
output_tar_ball="$( readlink -f "$( dirname "$5" )" )/$( basename "$5" )"

tmp_dir=`mktemp -d`    # Temporary directory for compilation
cd $server_dir
cp -r . $tmp_dir
cd $tmp_dir
rm -rf mac windows
mv linux/* .
rm -rf linux
$gepython_dir/Python-2.7.5/bin/python -m compileall .
chmod 500 *.py
chmod 700 *.pyc
# The following script will be the self extracting installer. The tar ball will
# be concatenated to it to create a single bundle.
cat > $output_tar_ball << END_SCRIPT
set -e

usage_print=0
exit_status=0
case \$1 in
  -h|-help|--help|--usage) usage_print=1;;
  *) case \$# in
       2) true;;
       *) usage_print=1; exit_status=1;;
     esac
esac

case \$usage_print in
  1)
  cmd="\$( basename \$0 )"
  echo "Usage   : bash \$cmd server_dir data_dir"
  echo "Example : bash \$cmd portable/server portable/data"
  echo "          Puts the server executable files to server_dir and default demo.glb to data_dir."
  echo "          You can start the server by typing"
  echo "            cd server_dir; ./gepython/Python-2.7.5/bin/python portable_server.py"
  echo ""
  exit \$exit_status
  ;;
esac

complete_name="\$( readlink -f "\$( dirname "\$0" )" )/\$( basename "\$0" )"
# readlink -f needs all but the last component to pre-exist.
mkdir -p "\$1" "\$2"  # So create the directories first as given by user.
server_dir="\$( readlink -f "\$( dirname "\$1" )" )/\$( basename "\$1" )"
data_dir="\$( readlink -f "\$( dirname "\$2" )" )/\$( basename "\$2" )"

cat > \$server_dir/portable.cfg << END_CFG
port 9335
globes_directory \$data_dir
database file
globe_name tutorial_sf.glb
END_CFG

cat > \$server_dir/remote.cfg << END_CFG
port 9335
portable_server __portable_server_ip__:9335
END_CFG

cd \$server_dir
tail --lines=+50 \$complete_name | tar xfp -  # This is the self extraction step
find *.glb -prune -type f -exec chmod 400 {} \; -exec mv -f {} \$data_dir \;
chmod go-rwx \$server_dir \$data_dir \$server_dir/portable.cfg \$server_dir/remote.cfg
echo "Installed GEE portable server successfully."
exit 0
END_SCRIPT
cp -rp $gepython_dir .
cp -p `find $gepython_dir -name "*.so" -exec ldd {} \; | grep "REL-x86_64" | cut -f2 -d'>' | cut -f2 -d' ' | sort | uniq` gepython/Python-2.7.5/lib/
cp -p "$globe_dir"/*.glb .
mkdir tornado_install
cd tornado_install
cat $tornado_tar_ball | tar xzf -
cd *
../../gepython/Python-2.7.5/bin/python setup.py build
../../gepython/Python-2.7.5/bin/python setup.py install --prefix ../../gepython/Python-2.7.5 --exec-prefix ../../gepython/Python-2.7.5
cd ../..
rm -rf tornado_install
find -type f ! -name "*.pyc" ! -name "*.pyo" -exec chmod a-w {} \;
chmod -R go-rwx .
tar cvf - . >> $output_tar_ball
cd ..
rm -rf $tmp_dir
chmod +x $output_tar_ball
