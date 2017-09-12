#!/bin/bash

# This script creates archive files that can be distributed as a release. The
# automatically generated files on the GitHub releases page do not include the
# files stored in Git LFS, so this process is required to create working
# archives that we can publish on the GitHub releases page.

# This script places the generated archives in the current directory.  Because
# this script downloads and packages a large repo it may take a long time to
# run.

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <branch>"
  echo "where <branch> is a branch or tag that defines the release to be archived."
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
TAR_FILE=${REPO_NAME}.tar.gz
ZIP_FILE=${REPO_NAME}.zip
RUN_DIR=`pwd`

# Make a temporary directory
CLONE_DIR=`mktemp -d`

# Download the repository
cd "${CLONE_DIR}"
git clone -b "${1}" --depth 1 git@github.com:google/${REPO_NAME}.git || exit

# Remove the git-related files from the repo
cd "${REPO_NAME}"
FILES=`find . -name ".git*"`
for FILE in ${FILES}; do
  rm -Rf "${FILE}"
done

# Create the archives
cd "${CLONE_DIR}"
tar czf "${TAR_FILE}" "${REPO_NAME}"
zip -rq "${ZIP_FILE}" "${REPO_NAME}"

# Clean up
mv "${TAR_FILE}" "${RUN_DIR}"
mv "${ZIP_FILE}" "${RUN_DIR}"
cd
rm -Rf "${CLONE_DIR}"

