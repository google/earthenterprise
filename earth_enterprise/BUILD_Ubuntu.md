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
sudo apt install git-lfs
```

## Install Required Packages
Run the following command:
```
sudo apt-get install \
    alien autoconf automake bison++ bisonc++ cmake doxygen dpkg \
    flex freeglut3-dev g++ gcc gettext \
    libc6 libc6-dev libcap-dev libfreetype6 libfreetype6-dev \
    libgdbm-dev libgeos-dev libgeos++-dev libgif-dev libgtest-dev \
    libjpeg-dev libjpeg8-dev libmng-dev libogdi3.2-dev \
    libperl4-corelibs-perl libpng12-0 libpng12-dev libpq-dev libproj-dev \
    libstdc++6 libtool \
    libx11-dev libxcursor-dev libxerces-c-dev libxft-dev libxinerama-dev \
    libxml2-dev libxml2-utils libxmu-dev libxrandr-dev libyaml-cpp-dev \
    openssl \
    python-dev python-imaging python-psycopg2 python-setuptools python2.7 \
    python2.7-dev python-git \
    scons shunit2 swig xorg-dev zlib1g-dev
```
For Ubuntu 14, in addition of the above command, run this extra one:
```
sudo apt install libboost-all-dev
```
