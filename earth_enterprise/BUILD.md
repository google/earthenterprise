If you want to build, install, or run Portable server, see the [Portable Server page](https://github.com/google/earthenterprise/wiki/Portable-Server).

# Building Earth Enterprise Fusion and Server

Building is currently supported on 64-bit versions of Ubuntu 14.04 LTS, Ubuntu 16.04 LTS, RHEL 6, RHEL 7, and CentOS 7.


## GEE 5.2.1 Build Prerequisites (All platforms) 

1. Setup required Tools and Dependencies
    * git 1.7.1+
    * git lfs 
    * gcc 4.8.x
    * scons 2.0.x+
    * python 2.6.x+ and Python 2.7.x+.  Python 3.0+ is not supported. 
    
    Different operating systems have different means to setup up these dependencies. For Linux build environments, see either the [Redhat and Centos Setup Instructions](./BUILD_RHEL_Centos.md)  
or the [Ubuntu Setup Instructions](./BUILD_Ubuntu.md) for those specific platforms on how to setup the dependencies, tools and compilers. 
 
1. Clone the EarthEnterprise repository in your build environment
    
    ___Note: for development you should follow the instructions on the [Git Contributions](https://github.com/google/earthenterprise/wiki/Git-Contributions) page to clone the GEE repo to your personal fork, so that you can submit git pull-requests back to this repo.___
           
    * Method 1: Clone and download LFS files in one step (may be slower and more error prone).
    
    ```
    git clone git@github.com:google/earthenterprise.git
    ```
    * Method 2: Clone and download LFS files in two steps
    ```
    GIT_LFS_SKIP_SMUDGE=1 git clone git@github.com:google/earthenterprise.git
    cd earthenterprise
    git lfs pull
    ```

1. In the build instructions below, the scons commands for building GEE/Fusion have the following options:
    * `internal=1` - Build using non-optimized code, best for development and debugging
    * `optimize=1` - Build using optimized code, but with some debugging information
    * `release=1` - Build a release using optimized code and no debugging information
    * `-j#` - Specifies the number of simultaneous build jobs to use. Replace # with an integer. It should roughly match the number of processing cores available
    * `--debug=stacktrace` - If there is an error within a scons script, this will give you more detailed information on how to debug it
    * `--config=force` - If you accidentally delete the .sconf_temp directory or make some changes to your system build libraries, use this to force the configuration to run again, otherwise the scons build may complain about missing libraries
1. Build Earth Enterprise Fusion and Server:
    ```
    cd earthenterprise/earth_enterprise
    scons -j8 release=1 build
    ```
1. Run unit tests (note that the `REL` part of the path will vary if you use `internal=1` or `optimize=1` instead
of `release=1`).
    ```
    cd src/NATIVE-REL-x86_64/bin/tests
    ./RunAllTests.pl
    ```
   
## Install Fusion and Earth Server
For information on how to install Fusion and/or Earth Server, see [Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server)

