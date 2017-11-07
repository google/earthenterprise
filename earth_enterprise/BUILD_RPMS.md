# Installing via Package Managers
This document describes how to build and install Open GEE packages (i.e. RPMs, and later, DEBs).  Packages must be built on the same platform that they are installed on to avoid library errors.  This document assumes you have development system of the target platform set up and ready to build Open GEE.

Currently only building RPM packages are supported.  To begin creating opengee RPMs, you first start with a normal Open GEE build environment following the existing [build instructions](https://github.com/google/earthenterprise/wiki/Build-Instructions), such as those already found in **BUILD.md**.  You should test this environment building a **stage_install** to verify everything builds correctly before attempting to create RPMs.  You should **NOT** use the ```install_server.sh``` or ```install_fusion.sh``` scripts.

## Prerequisites
The RPMs are built using gradle, and gradle requires Java.  On Ubuntu (16.04) one can use ```apt-get install default-jdk rpm```.  The minimum requirement is Java 7 JDK.  You can build RPMs on Ubuntu, and therefore test the build process itself there, but you cannot actually install the resulting RPMs anywhere.  To create RPM's that will install on Centos or RHEL 6, you must build your RPM's on the same platform.

On Centos or RHEL you should be able to do a ```yum install java-1.7.0-openjdk-devel```.  There are no other special prerequesets, as the scons build target pulls down osPackage and dependencies as needed before creating rpms.


## Creating RPMs
A new scons target was added to make RPMs, **package_install**, that can be ran from the *earth_enterprise* subdirectory.  This can be used after **stage_install** to verify the build, as it uses the stage install directory.  This will setup the stage install directory, install gradle if needed, and then uses gradle to generate RPMs.  The RPMs are created in the *rpms/build/distributions* subdirectory.  A typical invocation for building release RPMs might be ```scons -j8 release=1 package_install```.  All other files related to RPM generation are found in the *rpms* sub-directory as well.  Scripts for pre/post install operations are found under the related rpm directory in *rpms* and common templates for package script generation in *rpms/shared*.

Currently a core RPM is created for **opengee-postgis**, and a common RPM for third party dependencies called **opengee-common**.  These are required installs.  A separate RPM is also produced for **opengee-server** and **opengee-fusion**, which can then each be used independently, much like the original ```install_server.sh``` and ```install_fusion.sh``` scripts allowed.

## Installing RPMs

After using the scons **package_install** target completes you can install and test the RPMs you have created on Centos or RHEL.  To do this, change to the *rpms/build/distributions* directory.  You can then use ```rpm -Uhv opengee-postgis-*.rpm``` and ```rpm -Uhv opengee-common-*.rpm``` to get the base dependencies installed, in that order.  You can then install ```rpm -Uhv opengee-fusion-*.rpm``` and/or ```rpm -Uhv opengee-server-*.rpm``` as desired, and then you can test the RPM packaged opengee fusion and server.

The RPMs install opengee to the same */opt/google* path that the default install scripts do.  If you have already installed fusion or server from the git sources manually, it probably makes sense to use the ```src/installers/uninstall_fusion.sh``` and ```src/installers/uninstall_server.sh``` first before installing the RPM packages.  The RPMs should be able to migrate an existing manual install of opengee with the packaged version, but at the very least you should back up data if you are going to try such a migration.

While it is possible to migrate from a manual install of opengee to a RPM one, converting an RPM install back to a manual install is not safe.  This is because the RPM packaging system may remove key directories and files when the associated opengee packages are removed.  Again, make sure you have made backups of any critical data you may need before attempting this.