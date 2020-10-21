#!/bin/bash

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

INSTALL_PREFIX="/usr"
STARTING_DIR="${0%/*}"

SOURCE_DIR="$STARTING_DIR/../earth_enterprise/third_party/python"
SOURCE_27="$SOURCE_DIR/Python-2.7.18.tgz"
SOURCE_38="$SOURCE_DIR/Python-3.8.6.tgz"

TMP_WORKSPACE="/tmp/opengee_python_builds"
TMP_27="$TMP_WORKSPACE/Python-2.7.18"
TMP_38="$TMP_WORKSPACE/Python-3.8.6"

if [ ($# -eq 0) -o ($1 == "install") ]; then 
  mkdir $TMP_WORKSPACE;

  # Check if Python2.7 is installed
  if [ -f "/usr/bin/python2.7" ]; then
    echo "Python 2.7 is already installed, skipping...";
  else
    # Untar, build, and install python 2.7.
    mkdir $TMP_27;
    tar -xzf $SOURCE_27 -C $TMP_WORKSPACE;
    cd $TMP_27;
    ./configure --prefix="$INSTALL_PREFIX" --enable-optimizations;
    make;
    make altinstall;
  fi

  cd $STARTING_DIR;

  # Check if Python3.8 is installed
  if [ -f "/usr/bin/python3.8" ]; then
    echo "Python 3.8 is already installed, skipping...";
  else
    # Untar, build, and install python 3.8.
    mkdir $TMP_38;
    tar -xzf $SOURCE_38 -C $TMP_WORKSPACE;
    cd $TMP_38;
    ./configure --prefix="$INSTALL_PREFIX" --enable-optimizations;
    make;
    make altinstall;
  fi
fi

if [ $1 == "clean" ]; then
  # Run clean on individual builds, and then remove the dir.
  if [ -d $TMP_27 ]; then
    cd $TMP_27;
    make clean;
    make distclean;
  fi 
  
  if [ -d $TMP_38 ]; then
    cd $TMP_38;
    make clean;
    make distclean;
  fi
  
  rm -r $TMP_WORKSPACE;
fi


