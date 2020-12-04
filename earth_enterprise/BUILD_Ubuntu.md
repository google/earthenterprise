# GEE Build Setup for Ubuntu

## Install Git

Install the system default version:

```bash
sudo apt-get install git
```

Alternatively, install the latest version:

```bash
sudo add-apt-repository ppa:git-core/ppa
sudo apt-get update
sudo apt-get install git
```

## Install Git LFS

Execute the following:

```bash
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
sudo apt-get install git-lfs
```

## Install Required Packages
Run the following command:
```
sudo apt-get install \
    alien autoconf automake bison++ bisonc++ cmake doxygen dpkg \
    flex freeglut3-dev g++ gcc gettext \
    libboost-all-dev libc6 libc6-dev libcap-dev libfreetype6 libfreetype6-dev \
    libgdbm-dev libgeos-dev libgeos++-dev libgif-dev libgtest-dev \
    libjpeg-dev libjpeg8-dev libmng-dev libogdi3.2-dev \
    libperl4-corelibs-perl libpng12-0 libpng12-dev libpq-dev libproj-dev \
    libstdc++6 libtool libffi-dev \
    libx11-dev libxcursor-dev libxerces-c-dev libxft-dev libxinerama-dev \
    libxml2-dev libxml2-utils libxmu-dev libxrandr-dev libyaml-cpp-dev \
    openssl libpcre3 libpcre3-dev \
    scons shunit2 xorg-dev zlib1g-dev
```
## Install Python

In order to build OpenGEE, both Python 2.7 and 3.8 are needed (to facilitate transition to 3.8). If you have access to both through public repos, we suggest installing them that way. Python 2.7 is available from the official repos, so we install it from there:

```bash
sudo apt-get install python2.7 python2.7-dev python-pip
```

The following script will build and install Python 2.7 and Python 3.8 from source if they are needed:
 
```bash
sudo ./../scripts/install_python.sh
```

Additional python packages are needed as well:

```bash
sudo python2.7 -m pip install --upgrade pip==19.0
sudo python2.7 -m pip install argparse setuptools defusedxml GitPython Pillow unittest2 lxml psycopg2-binary
sudo python3.8 -m pip install --upgrade pip
sudo python3.8 -m pip install argparse setuptools defusedxml GitPython Pillow unittest2 lxml psycopg2-binary
```