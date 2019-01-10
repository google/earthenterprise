# GEE Build Setup for CentOS 6, CentOS 7, RHEL 6, and RHEL 7

## Enable Additional Repositories

Specific Yum repositories need to be enabled for different distributions to
install the required development tools.

### CentOS 7

```bash
sudo yum install epel-release
```

### RHEL 7

```bash
sudo yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
```

### CentOS 6

```bash
sudo wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
sudo yum install -y epel-release
```

### RHEL 6

__NOTE:__ The EPEL URL below assumes that your RHEL 6 installation has
the latest updates.

```bash
# For RHEL 6 Workstation:
sudo subscription-manager repos --enable=rhel-x86_64-workstation-dts-2

# For RHEL 6 Server:
sudo subscription-manager repos --enable=rhel-server-dts2-6-rpms

# For all RHEL 6 Editions:
sudo subscription-manager repos --enable=rhel-6-server-optional-rpms
sudo yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-6.noarch.rpm
```

## Install Git

Install the system default version:

```bash
sudo yum install -y git
```

Alternatively, install the latest version from the IUS repository (recommended).
See the [Getting Started](https://ius.io/GettingStarted/) page to find the RPM
URL for your system. Use this URL below:

```bash
sudo yum install -y wget
cd /tmp

# CentOS 7
wget https://centos7.iuscommunity.org/ius-release.rpm
sudo yum install -y ius-release.rpm
sudo yum install -y git2u-all
```

## Install Git LFS

Execute the following commands:

```bash
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh \
  | sudo bash
sudo yum install git-lfs
```

## GCC 4.8

For all versions of CentOS and RHEL, install the standard development/build tools:

```bash
yum install ant bzip2 doxygen gcc-c++ patch python-argparse python-setuptools \
  swig tar
```

For CentOS 6 and RHEL 6, also install the devtoolset toolchain.

### CentOS 6

```bash
yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-toolchain devtoolset-2-gcc-gfortran
```

### RHEL 6

```bash
sudo yum install -y devtoolset-2-gcc devtoolset-2-binutils \
  devtoolset-2-toolchain devtoolset-2-gcc-c++ devtoolset-2-gcc-gfortran
```

The GCC 4.8 installation will be located in the `/opt/rh/devtoolset-2/root/usr/bin/` directory.

The GEE build scripts will detect this compiler automatically. However, if you
wish to switch the environment to use GCC 4.8 in a shell, execute:

```bash
source /opt/rh/devtoolset-2/enable
```

## Install additional packages

__NOTE:__ If you are upgrading from GEE 5.1.3 or earlier, you must run this step
_after_ uninstalling older versions of GEE. Otherwise, some of the
prerequisites will be missing and the build will fail.

### CentOS 7 and RHEL 7

Execute:

```bash
sudo yum install -y \
  bison-devel cmake daemonize freeglut-devel \
  gdbm-devel geos-devel giflib-devel GitPython \
  libcap-devel libmng-devel libpng12-devel libX11-devel libXcursor-devel \
    libXft-devel libXinerama-devel libxml2-devel libXmu-devel libXrandr-devel \
  ogdi-devel openjpeg-devel openjpeg2-devel openssl-devel \
  perl-Alien-Packages perl-Perl4-CoreLibs proj-devel python-devel \
  rpm-build rpmrebuild rsync scons \
  xerces-c xerces-c-devel xorg-x11-server-devel yaml-cpp-devel zlib-devel
```

### CentOS 6 and RHEL 6
Execute:

```bash
sudo yum install -y \
  bison-devel  cmake daemonize freeglut-devel \
  gdbm-devel geos-devel gettext giflib-devel GitPython \
  libcap-devel libmng-devel libpng-devel libX11-devel libXcursor-devel \
    libXft-devel libXinerama-devel libxml2-devel libXmu-devel libXrandr-devel \
  ogdi-devel openjpeg-devel openjpeg2-devel openssl-devel \
  perl-Alien-Packages perl-Perl4-CoreLibs proj-devel python-devel python-unittest2 \
  rpm-build rpmrebuild rsync scons shunit2 \
  xerces-c xerces-c-devel xorg-x11-server-devel yaml-cpp-devel zlib-devel
```

If you encounter an error about git dependency conflicts, consider 
experimenting with the `--skip-broken` parameter.

## GTest 1.8

### CentOS 7 and RHEL 7

GTest is included in the EPEL and RHEL Extra Repositories. Install the RPM with:

```bash
yum install -y gtest-devel
```

### CentOS 6 and RHEL 6

You will need to compile, package, and install an updated version of GTest as an
RPM for RHEL6. This build process also depends on GCC 4.8, as does
the rest of the GEE build process.

To clone this Git repository and build the RPM on RHEL6, execute the following:

```bash
mkdir -p ~/opengee/rpm-build/
cd ~/opengee/rpm-build/

git clone https://github.com/thermopylae/gtest-devtoolset-rpm.git

cd gtest-devtoolset-rpm/
./bin/build.sh --use-docker=no
```

__NOTE:__ The GTest RPM can be built on other Linux systems using Docker. Simply
execute `build.sh` without the `--use-docker=no` argument. See the [README.md](https://github.com/thermopylae/gtest-devtoolset-rpm) for more details.

Install the RPM:

```bash
sudo yum install -y ./build/RPMS/x86_64/gtest-devtoolset2-1.8.0-1.x86_64.rpm
```
## shunit2

shunit2 is used for unit testing shell scripts in the GEE repository.
It is currently used only for testing package-building scripts.

### CentOS 7 and RHEL 7

The EPEL repositories for RHEL and CentOS 7 do not include shunit2.
If you want to run these tests on one of these platforms, you must install
shunit2 from the EPEL repositories for RHEL and CentOS 6.
To do so run the following:

```bash
sudo yum install -y http://download-ib01.fedoraproject.org/pub/epel/6/x86_64/Packages/s/shunit2-2.1.6-3.el6.noarch.rpm
```

### CentOS 6 and RHEL 6

shunit2 was installed in a previous step.
