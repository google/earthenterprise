#-*- Python -*-
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

# Return the package information for the current version of
# Google Earth Enterprise Fusion.

import sys
import os

def EnsurePathExists(dir):
    """Make sure dir exists, create it if it does not exist."""
    if not os.path.exists(dir):
        os.system("mkdir -p " + dir)

def ExecEcho(command):
    """Execute and echo the system command."""
    print(command)
    os.system(command)

def SyncPackages(build_agents_map, dest_dir):
    """Use rsync to copy packages from remote build machines to this machine.
    packages: list of PackageInfo for the packages to be copied
    build_agents_map: map of architectures to build agent rsync repositories to
      copy the packages from
    dest_dir: the directory into which the rpm packages will be copied"""

    # Clear the destination sync dir
    ExecEcho("rm -rf " + dest_dir)

    print("\nRsync'ing RPMS from build machines")
    # For each build agent, rsync the latest files to the current build machine.
    for arch in build_agents_map:
        src_dir = build_agents_map[arch]
        EnsurePathExists(dest_dir + arch)
        ExecEcho("rsync -rtl " + src_dir + " " + dest_dir + arch)

def DumpPackages(packages, package_dir, dest_dir):
    """Dump the contents of all packages to the specified directory.

    packages: the list of PackageInfo for the packages to be dumped
    package_dir: the package directory
    dest_dir: the directory to place the package contents.
    """

    print("\nDumping RPM Packages into InstallAnywhere...")
    for package in packages :
        DumpPackage(package, package_dir, dest_dir)

def DumpPackage (package, package_dir, parent_dir):
    """Dump the contents of a package to the specified directory.

    package: the PackageInfo for the package to be dumped
    package_dir: the package directory for rpms
    parent_dir: the directory to place the package contents.
    """

    current_dir = os.getcwd() # Save the current directory.

    # We need to get the absolute path to the RPM since we must cd into
    # the destination directory to dump the files from the package.
    package_path = os.path.abspath(package.RpmName(package_dir))

    # The package must be unpacked in a directory of the form:
    #   "./lib/arch/package-version.arch/"
    dest_dir = package.InstallAnywhereDir(parent_dir)

    # Clear the existing contents
    ExecEcho("rm -rf " + dest_dir + "*")

    EnsurePathExists(dest_dir)
    os.chdir(dest_dir);

    print("Dumping " + package.RpmName() + " to " + parent_dir + package.arch);
    os.system("rpm2cpio " + package_path + " | cpio -idm")
    os.chdir(current_dir) # cd back to the original working directory


def CopySrpms(packages, srpm_dir, dest_dir):
    """Copy any packages that are marked as 'SrpmRequired' to the dest_dir.
    packages: list of PackageInfo's (some of which may be marked 'SrpmRequired')
    srpm_dir: the parent dir of the SRPMS
    dest_dir: destination directory for the copies of the SRPMS
    """

    print("\nCopying SRPMS into InstallAnywhere...");
    # Clear the existing SRPMS
    os.system("rm -rf " + dest_dir + "*")

    EnsurePathExists(dest_dir)

    # Compile the "set" of srpms (unique entries only)
    srpm_set = set()
    for package in packages :
        if package.SrpmRequired():
            srpm = package.SrpmName(srpm_dir)
            srpm_set.add(srpm)

    for srpm in srpm_set:
        ExecEcho("cp " + srpm + " " + dest_dir)
