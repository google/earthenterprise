## Table of Contents

1. [Portable Server Prerequisites](#portable-server-prerequisites)
    1. [Build Prerequisites](#build-prerequisites)
    1. [Run-time Prerequisites](#run-time-prerequisites)
1. [Portable Server on Linux](#portable-server-on-linux)
    1. [Building on Linux](#building-on-linux)
    1. [Installing on Linux](#installing-on-linux)
    1. [Running on Linux](#running-on-linux)
1. [Portable Server on Windows](#portable-server-on-windows)
    1. [Building on Windows](#building-on-windows)
    1. [Installing on Windows](#installing-on-windows)
    1. [Running on Windows](#running-on-windows)
1. [Portable Server on Mac OS](#portable-server-on-mac-os)


# Portable Server Prerequisites

## Build Prerequisites:

    * Python 2.7
    * Python pexpect installed
    * Swig with support for Python (4.0.1 or later)
    * g++ (4.8 or later)

## Run-time prerequisites:

    * Python 2.7
    * Python tornado installed
    * Python Imaging Library (PIL) installed

#### On CentOS/RHEL 6:

    sudo yum -y install python27-pip
    sudo pip2.7 install tornado pillow

# Portable Server on Linux

## Building on Linux

Make sure you have Python, the `pexpect` Pip package, as well as `tornado`, and g++ installed. Swig is bundled for Linux.

### Getting a Build Environment

#### On Ubuntu:

    sudo apt-get install g++ python python-pexpect python-tornado libpython-dev python-psycopg2

#### On CentOS/RHEL 7:

    sudo yum -y install gcc-c++ python python-pip python-tornado python-psycopg2
    sudo pip install pexpect

#### On CentOS/RHEL 6:

If you're running an old version of a Red Hat distribution, such as Cent OS or RHEL 6, the packaged `g++` compiler will be too old.  Enable the EPEL package repository, and install `devtoolset2` to get a more recent compiler:

    sudo yum install devtoolset-2-toolchain
    sudo yum -y install ius-release
    sudo yum -y install gcc-c++ python27 python27-pip python27-devel
    sudo pip2.7 install tornado pexpect psycopg2-binary gitpython

### Building

Run

    cd earthenterprise/earth_enterprise/src/portableserver
    ./build.py

The build script will produce a compressed archive with a name that looks like `earthenterprise/earth_enterprise/src/portableserver/build/portableserver-linux-5.1.3-20170412.tar.gz`. The build date part of the file name will change depending on the day you build.

You can install the built Portable Server from this `.tar.gz` archive. 

To clean build files, run 

    cd earthenterprise/earth_enterprise/src/portableserver
    ./build.py --clean


## Installing on Linux

Portable Server is not currently packaged for Linux distributions by the GEE Open Source team. Instead, just extract the tarball generated in the [Building on Linux](#building-on-linux) step in a directory you want to run it from.  You could also create links, or start-up shell scripts for your convenience.

You need to have the Python interpreter and package dependencies listed in [Run-time Prerequisites](#run-time-prerequisites) set up and installed in order to run Portable Server.  If you carried out the [Building on Linux](#building-on-linux) step, you already have all of the required dependencies.


## Running on Linux

Change into the directory you extracted the built Portable Server tarball into (in the [Installing on Linux](#installing-on-linux) step).  Then, just start `server/portable_server.py`:

    1. cd portableserver-linux-5.1.3-20170412/server/ #(substituting your extracted directory)
    1. python portable_server.py

You can edit `portableserver-linux-5.1.3-20170412/server/portable.cfg` and `portableserver-linux-5.1.3-20170412/server/remote.cfg` for your configuration needs before starting the server.



# Portable Server on Windows

## Building on Windows

**Note:** It is important to ensure `g++` and `python` versions used are both 32 bit or both 64 bit.  Mixing them will lead to compilation/link errors which are not immediately obvious.

If you want to build a 64 bit version of portable server, you must install the 64 bit versions of g++ and python.

### Install a g++ Compiler

You can install [MinGW](https://sourceforge.net/projects/mingw-w64/) with a g++ compiler.  Ensure the architecture you select to install matches your desired build architecture.  If intending to distribute the build to other windows systems, be sure to select the `win` threads installation.  Compiling with g++ for posix threads will lead to additional run-time dependencies.

Make sure `g++` is set in your `PATH`.

### Install Swig with Python Support

1. Download a [Swig](http://www.swig.org/download.html) Zip for Windows.
2. Extract the Zip in a desired installation directory.
3. Add the installation directory you extracted to your `PATH`.


### Install Python

Download and install [Python](https://www.python.org/downloads/) the latest Python 2.7 package.  Ensure the python architecture matches your desired build architecture.

Once you have Python installed, make sure you have `pexpect` and `tornado` installed. E.g.:

    cd \Python27\Scripts
    pip install pexpect tornado

Add the directory you installed Python in to your `PATH`.


### Build Portable Server

Open a command prompt with `g++`, `swig` and `python` in your `PATH`.

E.g., on Windows your `PATH` may look like:

```
C:\swigwin-3.0.12;C:\Python27;C:\MinGW\bin;C:\Program Files\ . . .
```

Run

    cd earthenterprise\earth_enterprise\src\portableserver
    python build.py

The build script will produce a compressed archive with a name that looks like `earthenterprise\earth_enterprise\src\portableserver\build\portableserver-windows-5.1.3-20170412.zip`. The build date part of the file name will change depending on the day you build.

You can install the built Portable Server from this Zip archive.

To clean build files, run 

    cd earthenterprise\earth_enterprise\src\portableserver
    python build.py --clean


## Installing on Windows

We do not currently provide a Windows installer for Portable Server. Instead, just extract the built Portable Server from the Zip archive generated in the [Building on Windows](#building-on-windows) step in a directory you want to run it from.

You need to have the Python interpreter and packages listed in [Run-time Prerequisites](#run-time-prerequisites) set up and installed in order to run Portable Server.  If you carried out the [Building on Windows](#building-on-windows) step, you already have the required dependencies.


## Running on Windows

Change into the directory you extracted the built Portable Server Zip archive into (in the [Installing on Windows](#installing-on-windows) step). Then, just start `server\portable_server.py`:

    1. cd portableserver-windows-5.1.3-20170412\server\ #(substituting your extracted directory)
    1. python portable_server.py

You can edit `portableserver-windows-5.1.3-20170412\server\portable.cfg` and `portableserver-windows-5.1.3-20170412\server\remote.cfg` for your configuration needs before starting the server.



# Portable Server on Mac OS

Currently, building and running Portable Server on Mac OS has not been tested.

Previous versions have run on Mac OS, and the `build.py` script has untested logic for running the Mac OS build commands.  However, at present, you'll have to fix any problems you run into on Mac OS.

