# Installing via Package Managers

This document describes how to build and install Open GEE packages (i.e. RPMs,
and later, DEBs).  Packages should be built on the same platform that they are
installed on to avoid library errors.  This document assumes the development
system of the target platform is already set up and ready to build Open GEE.

Currently only building RPM packages are supported.  To begin creating Open GEE
RPMs requires a normal Open GEE build environment which can be setup by
following the existing
[build instructions](https://github.com/google/earthenterprise/wiki/Build-Instructions),
such as those already found in **BUILD.md**.  The build environment can be
tested by building **stage_install** before attempting to create RPMs.  The
```install_server.sh``` or ```install_fusion.sh``` scripts should **NOT** be
used.

## Prerequisites

The RPMs are built using Gradle, and Gradle requires Java.  On Ubuntu (16.04)
the JDK can be installed with ```apt-get install default-jdk rpm```.  The
minimum requirement is Java 7 JDK.  Building RPMs on Ubuntu is not supported.
To create RPM's that will install on CentOS or RHEL 6 requires building the
RPM's on the target platform.

To build RPMS on CentOS or RHEL requires installing the JDK with ```yum install
java-1.7.0-openjdk-devel```.  There are no other special prerequisites, as the
SCons build target pulls down osPackage and dependencies as needed before
creating RPMs.

Ensure that all required Debian or Yum repositories are enabled on your 
platform.  For EL6 and EL7 systems, the EPEL repository must be installed 
and enabled. 
RHEL satellite based systems also require the *optional* repository ( i.e., rhui-DISTRO-rhel-server-releases-optional) repository to provide some libraries 
including xerces-c and some perl libraries.

## Creating RPMs

A new SCons target was added to make RPMs, **package_install**, that can be run
from the *earth_enterprise* subdirectory.  This can be used after
**stage_install** to verify the build, as it uses the stage install directory.
This will setup the stage install directory, install Gradle if needed, and then
use Gradle to generate RPMs.  The RPMs are created in the
*rpms/build/distributions* subdirectory.  A typical invocation for building
release RPMs might be ```python2.7 /usr/bin/scons -j8 release=1
package_install```.  All other files related to RPM generation are found in the
*rpms* sub-directory as well. Scripts for pre/post install operations are found
under the related package directory in *rpms*, and common templates for package
script generation is found in *rpms/shared*.

Open GEE creates separate RPMs for Fusion and Server so that they can be
installed and used independently. It also creates separate RPMs for various
libraries, as detailed below:

* **opengee-postgis** - contains PostGIS libraries
* **opengee-common** - contains other third party dependencies and libraries
    used by both Open GEE Fusion and Server
* **opengee-fusion** - contains the files needed to run Open GEE Fusion, but
    does not contain any non-essential files like documentation, examples, and
    tutorials
* **opengee-server** - contains the files needed to run Open GEE Server, but
    does not contain any non-essential files like documentation, examples, and
    tutorials
* **opengee-extra** - contains documentation, examples, and tutorials for both
    Fusion and Server
* **opengee-full-fusion** - convenience RPM that installs **opengee-fusion**
    and **opengee-extra**
* **opengee-full-server** - convenience RPM that installs **opengee-server**
    and **opengee-extra**

## Installing RPMs

After the SCons **package_install** target completes the rpm command can
install and test the RPMs created on CentOS or RHEL. The simplest way to do
this is to create a `yum` repository for the Open GEE RPMs. However, you can
install the RPMs directly if you wish. To do this, change to the
*rpms/build/distributions* directory.  Use ```sudo rpm -Uhv
opengee-common-*.rpm``` and ```sudo rpm -Uhv opengee-postgis-*.rpm``` to get
the base dependencies installed, in that order.  Afterward use ```sudo rpm -Uhv
opengee-fusion-*.rpm``` and/or ```sudo rpm -Uhv opengee-server-*.rpm``` to
install and test the RPM packaged Open GEE Fusion and Server. If you want a
full install, next run ```sudo rpm -Uhv opengee-extra-*.rpm```, followed by
```sudo rpm -Uhv opengee-full-fusion-*.rpm``` and/or ```sudo rpm -Uhv
opengee-full-server-*.rpm```. Alternatively, you can run ```sudo yum install
*.rpm``` in the `rpms/build/distributions` directory to install all of the Open
GEE RPMs in one step. Note that installing a single RPM from a local directory
(such as ```sudo yum install opengee-server-*.rpm```) will not automatically
install other required Open GEE RPMs, such as **opengee-common** or
**opengee-postgis**, if they are not also specified and yum will refuse to
install if they were not previously installed. Using a `yum` repository avoids
this problem.

A note about needed enabled repositories: If the RPMs are to be installed on a
machine where they were not built, certain repos need enabled:

### CentOS 7

```bash
sudo yum install -y epel-release
```

### RHEL 7

```bash
sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
```

### CentOS 6

```bash
sudo yum install -y epel-release ius-release
```

### RHEL 6

__NOTE:__ The EPEL URL below assumes that your RHEL 6 installation has
the latest updates.

```bash
sudo subscription-manager repos --enable=rhel-6-server-optional-rpms
sudo yum-config-manager --enable ius
sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-6.noarch.rpm
```

The RPMs install Open GEE to the same */opt/google* path that the non-package
based install scripts did.  If Open GEE is already installed from the Git
sources manually, the ```uninstall_fusion.sh``` and ```uninstall_server.sh```
scripts can first be run before installing the RPM packages. While the RPMs
should be able to migrate an existing manual install of Open GEE with the
packaged version, backup any data before attempting such a migration.

## Removing RPMs
Removing RPMs can be performed using ```yum remove```.  To remove all all
opengee packages, use ```sudo yum remove "opengee-*"```. Either ```sudo yum
remove opengee-server``` or ```sudo yum remove opengee-fusion``` can be used to
selectively remove server or fusion.  The ```uninstall_server.sh``` and
```uninstall_fusion.sh``` scripts should **NOT** be used while RPM Open GEE
packages are installed.

While it is possible to migrate from a manual install of Open GEE to a RPM one,
converting an RPM install of Open GEE back to a manual install is not safe.
This is because the RPM packaging system may remove key directories and files
when the associated Open GEE packages are removed.  Backups should be made
of any critical data before attempting this.
