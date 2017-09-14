# GEE Build Setup for CentOS, RHEL 6 and RHEL 7     

## Enable additional repos
Different yum repositories need to be enabled for different distributions to install several required development tools.  

### CentOS 7 
```
sudo yum install epel-release
```

### RHEL 7  

```
sudo subscription-manager repos --enable=rhel-7-server-extras-rpms
sudo subscription-manager repos --enable=rhel-7-server-optional-rpms
sudo yum install -y epel-release
```    

### CentOS 6 
```
wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
sudo yum install -y epel-release
```

### RHEL 6
```
# For RHEL 6 Workstation:
sudo subscription-manager repos --enable=rhel-x86_64-workstation-dts-2

# For RHEL 6 Server:
sudo subscription-manager repos --enable=rhel-server-dts2-6-rpms

# For all RHEL 6 Editions:
sudo subscription-manager repos --enable=rhel-6-server-optional-rpms
sudo yum install https://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
```

_Note: the EPEL release number referenced above should match the RHEL version number you have installed_

## Install Git  
    
Install the system default version:
    
```
sudo yum install -y git
```
    
Or install the latest version from the IUS repo (recommended) [ [More Info] ](https://ius.io/GettingStarted/):
```
sudo yum install -y wget
cd /tmp
wget https://rhel7.iuscommunity.org/ius-release.rpm
sudo yum install -y ius-release.rpm
sudo yum install -y git2u-all
```        
        
## Install Git LFS
   
Execute the following commands
    
```
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | sudo bash
sudo yum install git-lfs
```
    
## GCC 4.8

### CentOS 7 and RHEL 7

To obtain GCC 4.8 on RHEL, you must install the "Development Tools" from RHSM: 
```
sudo yum --setopt=group_package_types=mandatory,default,optional groupinstall "Development Tools"
sudo yum install -y devtoolset-2-toolchain 
```


### CentOS 6 
```
yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-toolchain devtoolset-2-gcc-gfortran 
```

### RHEL 6

```
sudo yum install -y devtoolset-2-gcc devtoolset-2-binutils  devtoolset-2-toolchain devtoolset-2-gcc-c++ devtoolset-2-gcc-gfortran 
```

The GCC 4.8 installation will be located in the `/opt/rh/devtoolset-2/root/usr/bin/` directory.  

The GEE build scripts will detect this compiler automatically. However, if you wish switch the environment to use GCC 4.8 in a shell, run: 
```source /opt/rh/devtoolset-2/enable```


## Install additional packages
_Note: if you are upgrading from GEE 5.1.3 or earlier, you must run this step __after__ uninstalling older versions of GEE.  Otherwise, some of the prerequisites will be missing and your build will fail._

### CentOS 7 and RHEL 7
Execute: 
```
sudo yum install -y scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel perl-Alien-Packages  \
    openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel   \
    libmng-devel libcap-devel libpng12-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel  \
    openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel cmake rpm-build
```
### CentOS 6 and RHEL 6
Execute: 
```
sudo yum install -y scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel perl-Alien-Packages  \
    openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel   \
    libmng-devel libcap-devel libpng-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel  \
    openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel cmake rpm-build
```
If you get an error about git dependency conflicts, consider experimenting with the `--skip-broken` parameter.



## GTest 1.8
### CentOS 7 and RHEL 7
gtest is included in EPEL and RHEL Extra Repositories.  Install the RPM: 
```
yum install -y gtest-devel
``` 

### CentOS 6 and RHEL 6
You will need to compile, package and install an updated version of GTest as an RPM for RHEL6.   Note that this build process also depends on GCC 4.8, as does the rest of the GEE build process. 

To clone this git repo and build the RPM on RHEL6, execute the following: 
```
mkdir -p ~/opengee/rpm-build/
cd ~/opengee/rpm-build/
git clone https://github.com/thermopylae/gtest-devtoolset-rpm.git
cd gtest-devtoolset-rpm/
./bin/build.sh --use-docker=no
```
___Note: the gtest RPM can be built on other linux systems using docker - simply execute "build.sh without the `--no-docker` parameter.  See the [README.md](https://github.com/thermopylae/gtest-devtoolset-rpm) file for more details.___  
  
  Install the RPM: 
``` 
sudo yum install -y ./build/RPMS/x86_64/gtest-devtoolset2-1.8.0-1.x86_64.rpm
```  
