# Installing via Package Managers
This document describes how to build and install Open GEE packages (i.e. RPMs, and later, DEBs).  Packages must be built on the same platform that they are installed on to avoid library errors.  This document assumes you have development system of the target platform set up and ready to build Open GEE.

Currently only RPMs are supported.  You start with a normal Open GEE build environment
that you can use for development on Centos, RHEL, or Ubuntu machines via the existing [install scripts](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server).

## Prerequisites
The RPMs are built using gradle, and gradle requires Java.  On Ubuntu (16.04) one can use ```apt-get install default-jdk rpm```.  The minimum requirement is Java 7 JDK.  You can build RPMs on Ubuntu, and therefore test the build process itself there, but you cannot actually install the resulting RPMs anywhere.  To create RPM's that will install on Centos or RHEL 6, you must build your RPM's on the same platform.

On Centos or RHEL you should be able to do a ```yum install java-1.7.0-openjdk-devel```.  There are no other special prerequesets, as the scons build target pulls down osPackage and dependencies as needed before creating rpms.


## Create RPMs
A new scons target was added to make RPMs, **package_install**, that can be ran from the *earth_enterprise* subdirectory.  This can be used after **stage_install** to verify the build, as it uses the stage install directory.  This will setup the stage install directory, install gradle if needed, and then uses gradle to generate RPMs.  The RPMs are created in the *rpms/build/distributions* subdirectory.  A typical invocation for building release RPMs might be ```scons -j8 release=1 package_install```.  All other files related to RPM generation are found in the *rpms* sub-directory as well.  Scripts for pre/post install operations are found under the related rpm directory in *rpms* and common templates for package script generation in *rpms/shared*.

Currently a core RPM is created for **opengee-postgis**, and a common RPM for third party dependencies called **opengee-common**.  These are required installs.  A separate RPM is also produced for **opengee-server** and **opengee-fusion**, which can then each be used independently, much like the original ```install_server.sh``` and ```install_fusion.sh``` scripts allowed.