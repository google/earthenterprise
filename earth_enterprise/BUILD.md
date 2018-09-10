# Building Earth Enterprise Fusion and Server

__NOTE:__ If you want to build, install, or run Portable server, see the
[Portable Server page](https://github.com/google/earthenterprise/wiki/Portable-Server).

Building is currently supported for 64-bit versions of Ubuntu 14.04 LTS,
Ubuntu 16.04 LTS, RHEL 6, RHEL 7, CentOS 6, and CentOS 7.

## GEE 5.2.4 Build Prerequisites (all platforms)

1. Setup required Tools and Dependencies:

    * git 1.7.1+
    * git lfs
    * gcc 4.8.x
    * scons 2.0.x
    * python 2.6.x or python 2.7.x. __Python 3.0+ is not supported.__

Different operating systems have different means to setup up these dependencies.
For Linux build environments, see either the [Redhat and Centos Setup Instructions](./BUILD_RHEL_CentOS.md)
or the [Ubuntu Setup Instructions](./BUILD_Ubuntu.md) for those specific
platforms on how to setup the dependencies, tools, and compilers.

2. Clone the _earthenterprise_ repository in your build environment:

    __NOTE:__ For development you should follow the instructions on the [Git Contributions](https://github.com/google/earthenterprise/wiki/Development:-Git-Contributions)
    page to clone the GEE repo to your personal fork, so that you can submit
    git pull-requests back to this repo.

    * Method 1: Clone and download LFS files in one step (may be slower and
        more error prone):

        ```bash
        git clone git@github.com:google/earthenterprise.git
        ```

    * Method 2: Clone and download LFS files in two steps:

        ```bash
        GIT_LFS_SKIP_SMUDGE=1 git clone git@github.com:google/earthenterprise.git
        cd earthenterprise
        git lfs pull
        ```

3. In the build instructions below, the scons commands for building GEE/Fusion
    have the following options:

    * `internal=1`: Build using non-optimized code, best for development and
        debugging
    * `optimize=1`: Build using optimized code, but with some debugging
        information
    * `release=1`: Build a release using optimized code and no debugging
        information
    *  `build_folder=some_path`: Gives you full control to where build output is
        placed. Can be an absolute or relative path.  If it is a relative path then
        it will be relative to `earth_enterprise\src`.  Nothing is appended to the
        path so you have full control of the path.
    * `cache_dir=some_path`: (Experimental) Should be an absolute path used by SCons
        to cache build output.  Currently this parameter should be used for testing
        builds using SCons cache.  There are some open issues with using SCons cache
        and those working on the issues can use this option for testing their changes.
    * `-j#`: Specifies the number of simultaneous build jobs to use. Replace
        `#` with an integer. It should roughly match the number of processing
        cores available
    * `--debug=stacktrace`: If there is an error within a scons script, this
        will give you more detailed information on how to debug it
    * `--config=force`: If you accidentally delete the _.sconf_temp_ directory
        or make some changes to your system build libraries, use this to force
        the configuration to run again, otherwise the scons build may complain
        about missing libraries

4. Build Earth Enterprise Fusion and Server:

    ```bash
    cd earthenterprise/earth_enterprise
    scons -j8 release=1 build
    ```

5. Run unit tests (note: that the `REL` part of the path will vary if you use
    `internal=1` or `optimize=1` instead of `release=1` or the full path may be
    something completly different if you used `build_folder`):

    ```bash
    cd src/NATIVE-REL-x86_64/bin/tests
    ./RunAllTests.pl
    ```

## Install Fusion and Earth Server

For information on how to install Fusion and/or Earth Server, see
[Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server).
