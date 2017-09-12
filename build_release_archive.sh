#!/bin/bash

# This script creates archive files that can be distributed as a release. The
# automatically generated files on the github releases page do not include the
# files stored in git lfs, so this process is required to create working
# archives that we can publish on the github releases page.

# This script places the generated archives in the current directory.  Because
# this script downloads and packages a large repo it may take a long time to
# run.

# This script works under /tmp and it assumes that previous artifacts under /tmp
# (such as previously downloaded repos or previously created archives) can be
# safely deleted.

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <committish>"
  echo "where <committish> is a commit, branch, or tag that defines the release"
  echo "to be archived."
  exit 1
fi

# Verify that git lfs is installed
GIT_LFS_ENV=`git lfs env`
SMUDGE_HOOK=`expr match "${GIT_LFS_ENV}" '.*git config filter\.lfs\.clean = "\(.*\)".*'`
if [ -z "${SMUDGE_HOOK}" ]; then
  echo "You must install git lfs before running this script."
  exit
fi

REPO_NAME="earthenterprise"
CLONE_DIR="/tmp"

TAR_FILE=${REPO_NAME}.tar.gz
ZIP_FILE=${REPO_NAME}.zip
RUN_DIR=`pwd`

# Download the repository
cd ${CLONE_DIR}
rm -Rf ${REPO_NAME}
git clone git@github.com:google/${REPO_NAME}.git || exit
cd ${REPO_NAME} || exit
git checkout ${1} || exit

# Remove the git-related files from the repo
FILES=`find . -name ".git*"`
for FILE in ${FILES}; do
  rm -Rf ${FILE}
done

# Create the archives
cd ${CLONE_DIR}
rm -f ${TAR_FILE} ${ZIP_FILE}
tar czf ${TAR_FILE} ${REPO_NAME}
zip -rq ${ZIP_FILE} ${REPO_NAME}

# Clean up
mv ${TAR_FILE} ${RUN_DIR}
mv ${ZIP_FILE} ${RUN_DIR}
rm -Rf ${REPO_NAME}

