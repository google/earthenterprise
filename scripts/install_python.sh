#!/usr/bin/env bash

# Copyright 2020 the Open GEE Contributors.
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
# This script installs Python for OpenGEE using bundled source code.
# Currently both Python 2.7 and Python 3.8 are installed in order to 
# facilitate a transition to Python 3.
SELF_NAME=$(basename "$0")
INSTALL_PREFIX="/usr"
STARTING_DIR=$(dirname $(readlink -f "$0"))
PYTHON_VER2="Python-2.7.18"
PYTHON_VER3="Python-3.8.6"
SOURCE_DIR="$STARTING_DIR/../earth_enterprise/third_party"
SOURCE_27="$SOURCE_DIR/python_2/$PYTHON_VER2.tgz"
SOURCE_38="$SOURCE_DIR/python_3/$PYTHON_VER3.tgz"
TMP_WORKSPACE="/tmp/opengee_python_builds"
TMP_27="$TMP_WORKSPACE/$PYTHON_VER2"
TMP_38="$TMP_WORKSPACE/$PYTHON_VER3"
DO_INSTALL="yes"
INSTALL_SELECTED="no"
DO_CLEAN="no"
CLEAN_SELECTED="no"



# parse command line args

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    -h|--help)

    cat <<MSG
 Usage: $SELF_NAME [-i] [-c] [-h]
    -i|--install
      Perform the Python installation. 
      Default. Requires root permissions.

    -c|--clean
      Clean-up any lingering files. 
      Cannot be done in conjunction with install. 
      Requires root permissions.

    -h|--help
      Show this help message and exit

MSG
      exit 0
      ;;
    -i|--install)
      DO_INSTALL="yes"
      DO_CLEAN="no"
      INSTALL_SELECTED="yes"
      if [ $CLEAN_SELECTED == "yes" ]; then
        echo "Can only perform an install or a clean, not both"
        exit 1
      fi
      ;;
    -c|--clean)
      DO_CLEAN="yes"
      DO_INSTALL="no"
      CLEAN_SELECTED="yes"
      if [ $INSTALL_SELECTED == "yes" ]; then
        echo "Can only perform an install or a clean, not both"
        exit 1
      fi
      ;;
    *)
      echo "Unrecognized command-line argument: $1" >&2
      exit 1
      ;;
  esac
  shift
done

# id -u gives the user ID number. if 0, then root
if ! [ $(id -u) = 0 ]; then
  echo "$SELF_NAME Error: Must be root!"
  echo
  exit 1
fi 

if [ $DO_INSTALL == "yes" ]; then 
  mkdir $TMP_WORKSPACE

  # Check if Python2.7 is installed
  if [ -f "/usr/bin/python2.7" ]; then
    echo "Python 2.7 is already installed, skipping..."
  else
    # Untar, build, and install python 2.7.
    mkdir $TMP_27
    tar -xzf $SOURCE_27 -C $TMP_WORKSPACE
    cd $TMP_27
    ./configure --enable-shared --prefix="$INSTALL_PREFIX"
    make && make altinstall
    sudo ldconfig
    sudo python2.7 -m ensurepip
  fi

  cd $STARTING_DIR

  # Check if Python3.8 is installed
  if [ -f "/usr/bin/python3.8" ]; then
    echo "Python 3.8 is already installed, skipping..."
  else
    # Untar, build, and install python 3.8.
    mkdir $TMP_38
    tar -xzf $SOURCE_38 -C $TMP_WORKSPACE
    cd $TMP_38
    ./configure --enable-shared --prefix="$INSTALL_PREFIX" && make && make altinstall
    sudo ldconfig
    sudo python3.8 -m ensurepip
  fi
fi

if [ $DO_CLEAN == "yes" ]; then
  # Run clean on individual builds, and then remove the dir.
  if [ -d $TMP_27 ]; then
    cd $TMP_27
    make clean && make distclean
  fi 
  
  if [ -d $TMP_38 ]; then
    cd $TMP_38
    make clean && make distclean
  fi
  
  rm -r $TMP_WORKSPACE
fi


