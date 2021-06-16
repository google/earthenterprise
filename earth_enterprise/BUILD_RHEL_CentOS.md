# GEE Build Setup for CentOS 6, CentOS 7, RHEL 6, and RHEL 7

## Enable Additional Repositories

Specific Yum repositories need to be enabled for different distributions to
install the required development tools.

### CentOS 7

```bash
sudo yum install -y epel-release
```

### RHEL 7

```bash
# Setup subscriptions for non-AWS RHEL 7
sudo subscription-manager repos --enable=rhel-7-server-optional-rpms

# For RHEL 7 in AWS
sudo yum-config-manager --enable rhel-7-server-rhui-optional-rpms

# For all RHEL 7
sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

```

### CentOS 6

```bash
sudo curl -o /etc/yum.repos.d/devtools-2.repo https://people.centos.org/tru/devtools-2/devtools-2.repo
sudo yum install -y epel-release ius-release
```

### RHEL 6

__NOTE:__ The EPEL URL below assumes that your RHEL 6 installation has
the latest updates.

__NOTE:__ Installing devtoolset-2 on RHEL6 in AWS requires a RedHat
subscription.

```bash
# For RHEL 6 Workstation:
sudo subscription-manager repos --enable=rhel-x86_64-workstation-dts-2

# For RHEL 6 Server:
sudo subscription-manager repos --enable=rhel-server-dts2-6-rpms

# For All RHEL 6
sudo subscription-manager repos --enable=rhel-6-server-optional-rpms
sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-6.noarch.rpm
sudo yum install -y https://repo.ius.io/ius-release-el6.rpm
```

## Install Git

It's recommended to install a recent version of Git from the [IUS repositories](https://ius.io),
but the older RedHat or Centos packages can also be used.

### RHEL 6 and CentOS 6

```bash
sudo yum install -y https://repo.ius.io/ius-release-el6.rpm
sudo yum install -y git224
```

### RHEL 7 and CentOS 7

```bash
sudo yum install -y https://repo.ius.io/ius-release-el7.rpm
sudo yum install -y git224
```

### (Optional) System default version - all platforms

Instead of installing git from the IUS repositories you can use the system
default version. This version of git is older than the IUS version.

```bash
sudo yum install -y git
```

## Install Git LFS - all platforms

OpenGEE uses Git LFS to store large binary files.  To install Git LFS, use the following commands:

```bash
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh \
  | sudo bash
sudo yum install -y git-lfs
```

## GCC 4.8


### RHEL 7 And CentOS 7
For all versions of CentOS and RHEL, install the standard development/build tools:

```bash
sudo yum install -y ant bzip2 doxygen gcc-c++ patch python python-argparse python-defusedxml python-setuptools tar
```

### RHEL6 and Centos 6

```bash
sudo yum install -y ant bzip2 doxygen gcc-c++ patch tar python27 python-argparse python27-defusedxml python27-setuptools
```

Also install the devtoolset toolchain.

```bash
sudo yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-toolchain
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

### RHEL 7 and CentOS 7

Execute:

```bash
sudo yum install -y \
  bison-devel boost-devel cmake daemonize freeglut-devel \
  gdbm-devel geos-devel giflib-devel glib2-devel GitPython gtk2-devel \
  libcap-devel libicu-devel libmng-devel libpng-devel libtiff-devel libX11-devel libXcursor-devel \
  libXft-devel libXinerama-devel libxml2-devel libXmu-devel libXrandr-devel \
  ogdi-devel openjpeg-devel openjpeg2-devel openssl-devel \
  perl-Alien-Packages perl-Perl4-CoreLibs proj-devel python-devel \
  rpm-build rpmrebuild rsync scons \
  xerces-c xerces-c-devel xorg-x11-server-devel yaml-cpp-devel zlib-devel
```

###  RHEL 6 and CentOS 6

Execute:

```bash
sudo yum install -y \
  bison-devel boost-devel cmake daemonize freeglut-devel \
  gdbm-devel geos-devel gettext giflib-devel gtk2-devel \
  libcap-devel libmng-devel libpng-devel libX11-devel libXcursor-devel \
  libXft-devel libXinerama-devel libxml2-devel libXmu-devel libXrandr-devel \
  ogdi-devel openjpeg-devel openjpeg2-devel openssl-devel pcre pcre-devel \
  proj-devel python27 glib2-devel libtiff-devel \
  python27-pip python27-devel python27-setuptools python-unittest2 \
  python-devel rpm-build rpmrebuild rsync scons shunit2 \
  xerces-c xerces-c-devel xorg-x11-server-devel yaml-cpp-devel zlib-devel
```

If you encounter an error about git dependency conflicts, consider
experimenting with the `--skip-broken` parameter.

## GTest 1.8

###  RHEL 7 and CentOS 7

GTest is included in the EPEL and RHEL Extra Repositories. Install the RPM with:

```bash
sudo yum install -y gtest-devel
```

If you prefer to build GTest from source, you can try the following procedure:

```bash
wget https://github.com/google/googletest/archive/refs/tags/release-1.8.0.tar.gz
tar xvf release-1.8.0.tar.gz
cd googletest-release-1.8.0 && mkdir build && cd build
cmake -DMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && make && sudo make install
cd .. && sudo ln -s `pwd`/googletest /usr/src/gtest
sudo ldconfig
```

### RHEL 6 and CentOS 6

You will need to compile, package, and install an updated version of GTest as an
RPM for RHEL6. This build process also depends on GCC 4.8, as does
the rest of the GEE build process.

To clone this Git repository and build the RPM on RHEL6, execute the following:

```bash
sudo yum install -y wget
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

### RHEL 7 and CentOS 7

The EPEL repositories for RHEL and CentOS 7 do not include shunit2.
If you want to run these tests on one of these platforms, you must install
shunit2 from the EPEL repositories for RHEL and CentOS 6.
To do so run the following:

```bash
sudo yum install -y http://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/s/shunit2-2.1.6-3.el6.noarch.rpm
```

### RHEL 6 and CentOS 6

shunit2 was installed in a previous step.

## Install Python 2.7 Packages

### RHEL 7 and CentOS 7

Python 2.7 is installed as a system default, so no additional packages are needed.

### RHEL 6 and CentOS 6

```bash
sudo pip2.7 install gitpython
```

### Building on fips-enabled machines

In some circumstances on stig-ed machines, where md5 cryptography is used, fips will prevent OpenGee from being built. When trying to build, a message similar to the following will be displayed

```bash
$ python2.7 /usr/bin/scons -j4 internal=1 build
scons: Reading SConscript files ...
scons: done reading SConscript files.
scons: Building targets ...
scons: *** [build] ValueError : error:060800A3:digital envelope routines:EVP_DigestInit_ex:disabled for fips
scons: building terminated because of errors.
```

## prerequisites

* grub2
* openssl 1.0.1+

# Disabling fips

Before beginning, make sure that fips is indeed enabled. 1 - enabled, 0 - disabled
```bash
$ sudo cat /proc/sys/crypto/fips_enabled
1
```

First remove all dracut-fips packages
```bash
$ sudo yum remove -y dracut-fips*
```

It is recommended that initramfs is backed up
```bash
$ sudo cp -p /boot/initramfs-$(uname -r).img /boot/initramfs-$(uname -r).backup
```
Then, recreate a new initramfs file

```bash
$ sudo dracut -f
```

Now, disable ``fips=1`` from the kernel command line. Do this, by modifying command line of the current kernel in ``grub.cfg``. This is done by adding the option ``fips=0`` to the `GRUB_CMDLINE_LINUX` line in ``/etc/default/grub`` e.g.

``GRUB_CMDLINE_LINUX="console=tty0 crashkernel=auto console=ttyS0,115200"``

Would become

``GRUB_CMDLINE_LINUX="console=tty0 crashkernel=auto console=ttyS0,115200 fips=0"``

Now, after making the change to ``/etc/default/grub``, rebuild ``grub.cfg``

```bash
$ sudo grub2-mkconfig -o /boot/grub2/grub.cfg
```

On UEFI machines, this would be done by:

```bash
$ sudogrub2-mkconfig -o /boot/efi/EFI/<redhat or centos>/grub.cfg
```

Reboot the system and then verify that fips has been disabled

```bash
$ cat /proc/sys/crypto/fips_enabled
0
```

# Enabling fips

First, check that fips is supported

```bash
$ openssl version
OpenSSL 1.0.2k-fips  26 Jan 2017
```
This should be 1.0.1 and above

Then, check that fips is disabled

```bash
$ cat /proc/sys/crypto/fips_enabled
0
```

It is recommended that `df -h` and `blkid` are backed up

```bash
$ blkid > /var/tmp/blkid_bkp.bak
$ df -h > /var/tmp/df_bkp.bak
```

Now, make sure that `PRELINKING` is disabled in `/etc/sysconfig/prelink`. In other words, make sure that `PRELINKING=no` exists in the aforementioned file.

Then, make fips active on the kernel

```bash
$ sudo yum -y install dracut-fips-aesni dracut-fips
```

It is recommended that the current initramfs is backed up

```bash
$ sudo cp -p /boot/initramfs-$(uname -r).img /boot/initramfs-$(uname -r).backup
```

At this point, recreate the initramfs file

```bash
$ sudo dracut -f
```

Make sure that the `GRUB_CMDLINE_LINUX` line in `/etc/default/grib.cfg` contains `fips=1`

Rebuild grub.cfg

```bash
$ sudo grub2-mkconfig -o /boot/grub2/grub.cfg
```

On UEFI machines, this would be done in the following manner

```bash
$ grub2-mkconfig -o /boot/efi/EFI/redhat/grub.cfg
```

Reboot and then confirm that fips has been enabled

```bash
$ cat /proc/sys/crypto/fips_enabled
1
```
