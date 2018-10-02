#!/bin/bash

# This script creates archive files that can be distributed as a release. The
# automatically generated files on the GitHub releases page do not include the
# files stored in Git LFS, so this process is required to create working
# archives that we can publish on the GitHub releases page.

# This script places the generated archives in the current directory.  Because
# this script downloads and packages a large repo it may take a long time to
# run.

# Example usage: to create archives for 5.2.0 (which is on the release_5.2.0)
# branch, run
# ./build_release_archive.sh release_5.2.0

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
git clone -b "${1}" "https://github.com/google/${REPO_NAME}.git" || exit

cd "${CLONE_DIR}"/earthenterprise/earth_enterprise/src
# generate version files based on tags within git
scons version_files
cd "${CLONE_DIR}"/earthenterprise
# remove all un-tracked files except for version.txt
git clean -f -d -x -e version.txt
cd "${CLONE_DIR}"

# Remove the git-related files from the repo
find "${REPO_NAME}" -name ".git*" -exec rm -Rf {} +

# Create the archives
tar czf "${TAR_FILE}" "${REPO_NAME}"
zip -rq "${ZIP_FILE}" "${REPO_NAME}"

# Clean up
mv "${TAR_FILE}" "${RUN_DIR}"
mv "${ZIP_FILE}" "${RUN_DIR}"
cd
rm -Rf "${CLONE_DIR}"

