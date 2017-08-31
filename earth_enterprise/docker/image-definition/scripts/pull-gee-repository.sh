#! /bin/bash

if [ -d earthenterprise -a -d earthenterprise/.git ]; then
    echo "The GEE Open Source repository has already been cloned. Pulling latest updates"
    cd earthenterprise
    git pull
    git lfs pull
else
    GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/google/earthenterprise.git
    cd earthenterprise
    git lfs pull
fi