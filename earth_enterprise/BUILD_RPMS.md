# Installing via Package Managers
This document describes how to build and install Open GEE packages (i.e. RPMs,
and later, DEBs).  Packages should be built on the same platform that they are
installed on to avoid library errors.  This document assumes the development
system of the target platform is already set up and ready to build Open GEE.

Currently only building RPM packages are supported.  To begin creating Open Gee
RPMs requires a normal Open GEE build environment which can be setup by
following the existing [build instructions](https://github.com/google/earthenterprise/wiki/Build-Instructions),
such as those already found in **BUILD.md**.  The build environment can be
tested by building **stage_install** before attempting to create RPMs.  The
```install_server.sh``` or ```install_fusion.sh``` scripts should **NOT** be
used.

## Prerequisites
The RPMs are built using Gradle, and Gradle requires Java.  On Ubuntu (16.04)
the jdk can be installed with ```apt-get install default-jdk rpm```.  The
minimum requirement is Java 7 JDK.  It is possible to build RPMs on Ubuntu, and
therefore test the build process itself there, but the resulting RPMs cannot be
installed anywhere.  To create RPM's that will install on CentOS or RHEL 6
requires building the RPM's on those same platforms.

To build RPMS on CentOS or RHEL requires installing the jdk with ```yum install
java-1.7.0-openjdk-devel```.  There are no other special prerequisites, as the
SCons build target pulls down osPackage and dependencies as needed before
creating RPMs.

## Creating RPMs
A new SCons target was added to make RPMs, **package_install**, that can be run
from the *earth_enterprise* subdirectory.  This can be used after
**stage_install** to verify the build, as it uses the stage install directory.
This will setup the stage install directory, install Gradle if needed, and
then uses Gradle to generate RPMs.  The RPMs are created in the
*rpms/build/distributions* subdirectory.  A typical invocation for building
release RPMs might be ```scons -j8 release=1 package_install```.  All other
files related to RPM generation are found in the *rpms* sub-directory as
well. Scripts for pre/post install operations are found under the related
package directory in *rpms*, and common templates for package script generation
is found in *rpms/shared*.

Currently a separate RPM is created for **opengee-postgis**, and a common RPM
for all other third party dependencies called **opengee-common**.  These are
required installs.  A separate RPM is also produced for **opengee-server** and
**opengee-fusion**, which can then each be installed and used independently,
much like the ```install_server.sh``` and ```install_fusion.sh``` scripts
allowed Open GEE Fusion and Server to be separately installed when building out
of the Git repo checkout directly.

## Installing RPMs

After the SCons **package_install** target completes the rpm command can
install and test the RPMs created on CentOS or RHEL.  To do this, change to the
*rpms/build/distributions* directory.  Use ```sudo rpm -Uhv
opengee-postgis-*.rpm``` and ```sudo rpm -Uhv opengee-common-*.rpm``` to get
the base dependencies installed, in that order.  Afterward use ```sudo rpm -Uhv
opengee-fusion-*.rpm```, and then ```sudo rpm -Uhv opengee-server-*.rpm``` to
install and test the RPM packaged Open GEE Fusion and Server.

Yum can also be used to install opengee RPM files, and will also automatically
install required system dependencies.  However, since the opengee packages are
not in a repository, using ```sudo yum install opengee-server-*.rpm``` or
```sudo yum install opengee-fusion-*.rpm``` will not automatically install
**opengee-common** or **opengee-postgis**, and yum will refuse to install if
they are not already installed.  All the rpm packages could be also installed
together using ```sudo yum install *.rpm``` in the `rpms/build/distributions`
directory.

The RPMs install Open GEE to the same */opt/google* path that the non-package
based install scripts did.  If Open GEE is already installed from the Git
sources manually, the ```uninstall_fusion.sh``` and ```uninstall_server.sh```
scripts can first be run before installing the RPM packages. While the RPMs
should be able to migrate an existing manual install of Open GEE with the
packaged version, backup any data before attempting such a migration.

## Removing RPMs
Removing RPMs can be performed using ```yum remove```.  To remove all all
opengee packages, use ```sudo yum remove opengee-postgis``` as all other
opengee packages depend on it, and this will automatically remove
**opengee-common**, **opengee-server**, and **opengee-fusion** if they are also
installed. Either ```sudo yum remove opengee-server``` or ```sudo yum remove
opengee-fusion``` can be used to selectively remove server or fusion.  The
```uninstall_server.sh``` and ```uninstall_fusion.sh``` scripts should **NOT**
be used while RPM Open GEE packages are installed.

While it is possible to migrate from a manual install of Open GEE to a RPM one,
converting an RPM install of Open GEE back to a manual install is not safe.
This is because the RPM packaging system may remove key directories and files
when the associated Open GEE packages are removed.  Backups should be made
of any critical data before attempting this.
