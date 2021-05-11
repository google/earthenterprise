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
    libboost-all-dev libc6 libc6-dev libcap-dev libffi-dev libfreetype6 libfreetype6-dev \
    libgdbm-dev libgeos-dev libgeos++-dev libgif-dev libgtest-dev \
    libjpeg-dev libjpeg8-dev libmng-dev libogdi3.2-dev \
    libperl4-corelibs-perl libpng12-0 libpng12-dev libpq-dev libproj-dev \
    libstdc++6 libtool libgif-dev libtiff-dev libgtk2.0-dev libglib2.0-dev \
    libx11-dev libxcursor-dev libxerces-c-dev libxft-dev libxinerama-dev \
    libxml2-dev libxml2-utils libxmu-dev libxrandr-dev libyaml-cpp-dev \
    openssl libpcre3 libpcre3-dev \
    shunit2 xorg-dev zlib1g-dev
```
## Install Python

In order to build OpenGEE, Python 3.8 is needed. If you have access to it through public repos, we suggest installing it that way.
If you do not have access, Python 3.8 can be built and installed by running the following from `earthenterprise`:
 
```bash
sudo ./scripts/install_python.sh
```

Additional python packages are needed as well:

```bash
sudo python3.8 -m pip install --upgrade pip
sudo python3.8 -m pip install argparse setuptools defusedxml GitPython Pillow unittest2 lxml psycopg2-binary scons
```
