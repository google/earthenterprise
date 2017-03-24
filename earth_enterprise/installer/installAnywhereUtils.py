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

# installAnywhereUtils
# Utilities for building an InstallAnywhere installer for
# Google Earth Enterprise Fusion.
import sys
import os
from packageUtils import EnsurePathExists, ExecEcho

def BuildInstallAnywhereTarBall(ge_fusion_version_basic,
                                ge_fusion_version,
                                installer_dir):
    """Build the InstallAnywhere installer for Google Earth Fusion and
    package it up into a tarball.

    ge_fusion_version: the version of fusion being built
    installer_dir: the location of the resulting installer
    """

    # Remember the current directory
    current_dir = os.getcwd()

    # Run "ant" to create the installer
    print("\nCreating InstallerAnywhere files for Fusion and Server using \"ant\"...")
    os.system("ant")

    # Create the package of files in installer_dir/GEEFusion-##-YYYYMMDD
    package_dir = "GEEFusion-" + ge_fusion_version

    dir = installer_dir + package_dir
    dir_bin = dir + "/.installer"
    if not os.path.exists(dir_bin):
      os.makedirs(dir_bin)

    print("Copying installer files into package " + dir)

    fusion = "build/fusion/Web_Installers/InstData/Linux/NoVM/GEFusion.bin "
    server = "build/server/Web_Installers/InstData/Linux/NoVM/GEServer.bin "
    ExecEcho("  cp -r " + fusion + server + dir_bin)

    scripts = "Scripts/fusion/* Scripts/server/* "
    srpms = "lib/SRPMS "
    ExecEcho("  cp -r " + scripts + srpms + dir)

    # Copy the installer utilities into the .installer directory
    scripts = "Scripts/utils/* "
    ExecEcho("  cp -r " + scripts + dir_bin)

    # Must replace FUSION_VERSION_NUMBER in Uninstaller scripts.
    ExecEcho("  sed -i 's/FUSION_VERSION_NUMBER/" + ge_fusion_version_basic +
             "/g' " + dir + "/*.sh")

    # Must copy the docs/landing_page/fusion/* to dist/docs/*
    # so that it is packaged in the top level folder of the installer.
    dist_docs_dir = dir + "/docs/"
    EnsurePathExists(dist_docs_dir)
    ExecEcho("cp -r " + "../docs/landing_page/fusion/* " + dist_docs_dir)

    # Must remove all .svn directories from the dist before continuing
    os.system("find "  + dir + " -name .svn -type d -exec rm -rf {} \; 2>/dev/null")

    # CD into the installer directory to tarball the package
    os.chdir(installer_dir)
    # Make the shell and binaries executable.
    print("Making the installer scripts executable...")
    ExecEcho("chmod +x " + package_dir + "/*.sh")
    ExecEcho("chmod +x " + package_dir + "/.installer/*")

    # List the package_dir directory for log purposes.
    ExecEcho("ls -l " + package_dir)

    print("Creating installer tarball " + dir + ".tar.gz")
    tarball_path = os.getcwd() + "/" + package_dir + ".tar.gz"
    ExecEcho("  tar czf " + package_dir + ".tar.gz " + package_dir)
    ExecEcho("chmod a+r " + tarball_path)

    os.chdir(current_dir)
    # Clean up the untar'd package.
    print("Cleaning up temp directory " + dir)
    ExecEcho("rm -rf " + dir)

    print("Install Complete: " + tarball_path);

def BuildFusionToolsTarBall(ge_fusion_version_basic,
                            ge_fusion_version,
                            installer_dir):
    """Build the InstallAnywhere installer for Google Earth Fusion Tools
    package it up into a tarball.

    ge_fusion_version: the version of fusion being built
    installer_dir: the location of the resulting installer
    """

    # Remember the current directory
    current_dir = os.getcwd()

    # Create the package of files in installer_dir/GEEFusion-##-YYYYMMDD
    package_dir = "GEEFusionTools-" + ge_fusion_version

    dir = installer_dir + package_dir
    dir_bin = dir + "/.installer"
    if not os.path.exists(dir_bin):
      os.makedirs(dir_bin)

    print("Copying installer files into package " + dir)

    fusion_tools = "build/tools/Web_Installers/InstData/Linux/NoVM/GEFTools.bin "
    ExecEcho("  cp -r " + fusion_tools + dir_bin)

    scripts = "Scripts/tools/* "
    ExecEcho("  cp -r " + scripts + dir)

    # Copy the installer utilities into the .installer directory
    scripts = "Scripts/utils/* "
    ExecEcho("  cp -r " + scripts + dir_bin)

    # Must replace FUSION_VERSION_NUMBER in Uninstaller scripts.
    ExecEcho("  sed -i 's/FUSION_VERSION_NUMBER/" + ge_fusion_version_basic +
             "/g' " + dir + "/*.sh")

    # CD into the installer directory to tarball the package
    os.chdir(installer_dir)
    # Make the shell and binaries executable.
    print("Making the installer scripts executable...")
    ExecEcho("chmod +x " + package_dir + "/*.sh")
    ExecEcho("chmod +x " + package_dir + "/.installer/*")

    # List the package_dir directory for log purposes.
    ExecEcho("ls -l " + package_dir)

    print("Creating installer tarball " + package_dir + ".tar.gz")
    tarball_path = os.getcwd() + "/" + package_dir + ".tar.gz"
    ExecEcho("  tar czf " + package_dir + ".tar.gz " + package_dir)
    ExecEcho("chmod a+r " + tarball_path)

    os.chdir(current_dir)
    # Clean up the untar'd package.
    print("Cleaning up temp directory " + dir)
    ExecEcho("rm -rf " + dir)

    print("Install Complete: " + tarball_path);

def HardLinkTutorial():
    """Make hard links of the tutorial data into the tmp_install/ directory."""
    # Remember the current directory
    current_dir = os.getcwd()
    os.chdir("../tutorial")
    scons_command = "scons installdir=../installer/tmp_install"
    # Make the shell and binaries executable.
    print("Hard Linking the Tutorial Data...")
    ExecEcho(scons_command + " -c")
    ExecEcho(scons_command)
    # Return to current directory
    os.chdir(current_dir)
