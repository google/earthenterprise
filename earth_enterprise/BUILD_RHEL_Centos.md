# GEE Build Setup for CentOS, RHEL 6 and RHEL 7     


## Install git  
    
Install the system default version:
    
```
sudo yum install git
```
    
Or install the latest version from the IUS repo (recommended) [ [More Info] ](https://ius.io/GettingStarted/):
```
sudo yum install -y wget
cd /tmp
wget https://rhel7.iuscommunity.org/ius-release.rpm
sudo yum install ius-release.rpm
sudo yum install git2u-all
```
        
        
## Install git lfs
   
Execute the following commands
    
```
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | sudo bash
sudo yum install git-lfs
```



## Enable additional repos
Different yum repositoies need to be enabled for different distributions to install several required development tools.  

### CentOS 7 
```
sudo yum install epel-release
```

### RHEL 7  

_note:_ If the first command doesn't work, try the alternate methods listed [here](https://www.cyberciti.biz/faq/installing-rhel-epel-repo-on-centos-redhat-7-x/)
```
sudo subscription-manager repos --enable=rhel-7-server-optional-rpms
sudo subscription-manager repos --enable=rhel-7-server-optional-source-rpms
```    

### RHEL 6
```
sudo subscription-manager repos --enable=rhel-x86_64-workstation-dts-2
sudo subscription-manager repos --enable=rhel-6-server-optional-rpms
sudo subscription-manager repos --enable=rhel-6-server-optional-source-rpms
```
       
## GCC 4.8

### CentOS 7 and RHEL 7

To obtain GCC 4.8 on RHEL6, you must enable the "Development Tools" repository in the RHSM: 
```
sudo yum --setopt=group_package_types=mandatory,default,optional groupinstall "Development Tools"
sudo yum install scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel perl-Alien-Packages gtest-devel openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel libmng-devel libcap-devel libpng12-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel
```
### RHEL 6

```
sudo yum install devtoolset-2-gcc devtoolset-2-binutils  devtoolset-2-gcc-c++ devtoolset-2-gcc-gfortran
```

The GCC 4.8 installation will be located in the `/opt/rh/devtoolset-2/root/usr/bin/` directory.  

The GEE build scripts will detect this compiler automatically. However, if you wish switch the environment to use GCC 4.8 in a shell, run: 
```source /opt/rh/devtoolset-2/enable```

## GTest 1.8


### CentOS 7 and RHEL 7

gtest is included in EPEL and RHEL Extra Repositories.  Install the RPM: 
```
yum install gtest-devel
``` 

### RHEL 6
You will need to compile, package and install an updated version of GTest as an RPM for RHEL6.   Note that this build process also depends on GCC 4.8, as does the rest of the GEE build process. 

To clone this git repo and build the RPM on RHEL6, execute the following: 
```
mkdir -p /root/opengee/rpm-build
cd /root/opengee/rpm-build 
git clone https://github.com/thermopylae/gtest-devtoolset-rpm.git 
cd ./gtest-rhel6-devtoolset
./bin/build.sh --no-docker 
```
___Note: the gtest RPM can be built on other linux systems using docker - simple remove the ```--no-docker``` command.  See the [README.md](https://github.com/thermopylae/gtest-devtoolset-rpm) file for more details.___  
  
Install the RPM: 
``` 
yum install -y gtest-rhel6-devtoolset/build/RPMS/x86_64/gtest-devtoolset2-1.8.0-1.x86_64.rpm

```  

## Install additional packages

_Note: if you are upgrading from GEE 5.1.3 or earlier, you must run this step __after__ uninstalling older versions of GEE.  Otherwise, some of the prerequisites will be missing and your build will fail._

### CentOS 7 and RHEL 7
Execute: 
```
sudo yum install scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel perl-Alien-Packages  \
    openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel   \
    libmng-devel libcap-devel libpng12-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel  \
    openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel
```
### RHEL 6
Execute: 
```
sudo yum install scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel perl-Alien-Packages  \
    openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel   \
    libmng-devel libcap-devel libpng-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel  \
    openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel
```


If you get an error about git dependency conflicts, consider experimenting with the `--skip-broken` parameter.






