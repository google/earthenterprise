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

#
# Script to perform backup of files during uninstallation and
# update process of Server.
#

#!/bin/bash

if [ $# -eq 0 ]; then
  BACKUP_DIR="/var/opt/google/geserver-backups/"`date +%Y%m%d.%H%M%S`
else
  BACKUP_DIR=$1
fi

set -x

INSTALL_DIR="/opt/google"
PATH_TO_LOGS="/var/opt/google/log"

if [ ! -d $BACKUP_DIR ]; then
  mkdir -p $BACKUP_DIR
fi

# Copy gehttpd directory.
# Do not back up folder /opt/google/gehttpd/htdocs/cutter/globes.
# Do not back up folders /opt/google/gehttpd/{bin, lib, manual, modules}
rsync -rltpu $INSTALL_DIR/gehttpd $BACKUP_DIR --exclude bin --exclude lib --exclude manual --exclude modules --exclude htdocs/cutter/globes

# Copy other files.
cp -f /etc/init.d/gevars.sh $BACKUP_DIR
cp -rf /etc/opt/google/openldap $BACKUP_DIR

# Change the ownership of the backup folder.
chown -R root:root $BACKUP_DIR
