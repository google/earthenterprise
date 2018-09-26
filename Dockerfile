FROM ubuntu:16.04

USER root
   
RUN apt-get update && \
    apt-get install -y curl nano git && \
    curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && \
    apt install git-lfs && \
    apt-get install -y \
    alien autoconf automake bison++ bisonc++ cmake doxygen \
    flex freeglut3-dev g++ gcc gettext \
    libc6 libc6-dev libcap-dev libfreetype6 libfreetype6-dev \
    libgdbm-dev libgeos-dev libgeos++-dev libgif-dev libgtest-dev \
    libjpeg-dev libjpeg8-dev libmng-dev libogdi3.2-dev \
    libperl4-corelibs-perl libpng12-0 libpng12-dev libpq-dev libproj-dev \
    libstdc++6 libtool \
    libx11-dev libxcursor-dev libxerces-c-dev libxft-dev libxinerama-dev \
    libxml2-dev libxml2-utils libxmu-dev libxrandr-dev \
    openssl \
    python-dev python-imaging python-psycopg2 python-setuptools python2.7 \
    python2.7-dev python-git \
    scons shunit2 swig xorg-dev zlib1g-dev \
    g++ python-pexpect python-tornado libpython-dev python-pil

WORKDIR /data
