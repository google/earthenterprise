---
layout: page
title:  "Getting Started With Google Earth Enterprise"
icon: globe
---

Earth Enterprise is the open source release of Google Earth Enterprise, a geospatial application which provides the ability to build and host custom 3D globes and 2D maps. Earth Enterprise does not provide a private version of Google imagery that's currently available in Google Maps or Earth.

The application suite consists of three core components:

`Fusion` - Imports and processes imagery, vector and terrain source data

`Server` - Apache or Tornado-based server which hosts the private globes built by Fusion

`Portable` - A disconnected globe that allows you to access your data in a local-server configuration

`Client` - The Google Earth Enterprise Client (EC) and Google Maps Javascript API V3 used to view 3D globes and 2D maps, respectively.

## Installation

Earth Enterprise Fusion & Server currently run on 64-bit versions of the following operating systems:

* Red Hat Enterprise Linux versions 7.3, including the most recent security patches

* CentOS 7.3

* Ubuntu 14.04 LTS


#### Building Third Party Tools/Fusion on Ubuntu 14.04 LTS and RHEL 7

**Common steps**

Run these commands on either Ubuntu or RHEL/CentOS

1. Install git according to the instructions for your Linux OS below
2. Clone the gee-os repository with git:
3. In the build instructions below, the scons commands for building GEE/Fusion have the following options:

	* internal=1 - Build using non-optimized code, best for development and debugging
	* optimize=1 - Build using optimized code, but with some debugging information
	* release=1 - Build a release using optimized code and no debugging information
	* -j# - Specifies the number of simultaneous build jobs to use. Replace # with an integer. It should roughly match the number of processing cores available
	* --debug=stacktrace - If there is an error within a scons script, this will give you more detailed information on how to debug it
	* --config=force - If you accidentally delete the .sconf_temp directory or make some changes to your system build libraries, use this to force the configuration to run again, otherwise the scons build may complain about missing libraries

4. Build using the steps bellow for your target platform
5. Run unit tests

  {% highlight shell %}
  cd NATIVE-OPT-x86_64/bin/tests
  ./RunAllTests.pl
  {% endhighlight %}

Or run an individual test

#### Steps for Building Third Party Libraries on Ubuntu 14.04.5 LTS:

1.Install git: 
(installs version 1.9.1)

  {% highlight shell %}
  sudo apt-get install git 
  {% endhighlight %}

To get the most recent version of git, do the following (version 1.9.1 works for purposes of interacting with the 
repository, but if you want to download the latest version of git, the steps to do this have been outlined below):

  {% highlight shell %} 
  sudo -i add-apt-repository ppa:git-core/ppa apt-get update apt-get install git
  {% endhighlight %}
    
    
This will install version 2.11 Note: Be sure to exit "sudo" before proceeding

2.Install the following packages (if they’re not installed already):
	
  {% highlight shell %} 
  sudo apt-get install gcc g++ scons automake autoconf libperl4-corelibs-perl libtool xorg-dev doxygen python-dev 
  alien swig libgtest-dev libstdc++6 libxml2-dev libgtest-dev gettext libxinerama- 
  dev libxft-dev libxrandr-dev libxcursor-dev libgdbm-dev libc6 libc6-dev libmng-dev zlib1g-dev 
  libcap-dev libpng12-0 libpng12-dev freeglut3-dev flex libx11-dev python-dev bison++ bisonc++ libjpeg-dev 
  libjpeg8-dev python2.7 python2.7-dev libgtest-dev libogdi3.2-dev libgif-dev libxerces-c-dev libgeos-dev 
  libgeos++-dev libfreetype6 libfreetype6-dev python-imaging libproj-dev python-setuptools libgif-dev 
  libxerces-c-dev libcap-dev libpq-dev
  {% endhighlight %}

3.Build the third party library by running:

For reference: GEEDIR=/googleclient/geo/earth_enterprise

  {% highlight shell %}
  cd $GEEDIR/src scons -j4 optimize=1 third_party
  {% endhighlight %}
    
4. Build fusion/earth server

  {% highlight shell %}
  scons -j4 optimize=1
  {% endhighlight %}
     
#### Steps for building 3rd party tools and for RHEL 7

1. Install git 

  {% highlight shell %}
  sudo yum install git
  {% endhighlight %}

2.Install Development Tools

  {% highlight shell %}
  sudo yum --setopt=group_package_types=mandatory,default,optional groupinstall "Development Tools"
  {% endhighlight %}
  
3.Install additional packages 

  {% highlight shell %}
  sudo yum install scons perl-Perl4-CoreLibs xorg-x11-server-devel python27-python-devel perl-Alien-Packages gtest-devel 
  openssl-devel libxml2-devel libXinerama-devel libXft-devel libXrandr-devel libXcursor-devel gdbm-devel libmng-devel 
  libcap-devel libpng12-devel libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel 
  openjpeg-devel openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c-devel
  {% endhighlight %}

4.Build third-party tools 

{% highlight shell %}
  GEEDIR=/googleclient/geo/earth_enterprise
  cd $GEEDIR/src scons -j6 optimize=1 third_party
{% endhighlight %}

5.Build Fusion/Earth Server

{% highlight shell %}
scons -j6 optimize=1 third_party
{% endhighlight %}

#### Build and deploy if you have access to the 5.1.3 installer

A build and deploy script is provided that can simplify the build and installation process if you have the GEE and 
Fusion 5.1.3 installers. In the future this script will be updated to work without the previous installers.

Install GEE and Fusion 5.1.3

  {% highlight shell %}
  cd $GEEDIR/src
  ./tmp/build_and_deploy_gee.sh --build
{% endhighlight %}

## How to contribute

We'd love to accept your patches and contributions to this project. There are just a few small guidelines you need to follow.

#### Contributor License Agreement

Contributions to this project must be accompanied by a Contributor License Agreement. You (or your employer) retain the copyright to your contribution, this simply gives us permission to use and redistribute your contributions as part of the project. Head over to [https://cla.developers.google.com/](https://cla.developers.google.com/) to see your current agreements on file or to sign a new one.

You generally only need to submit a CLA once, so if you've already submitted one (even if it was for a different project), you probably don't need to do it again.

#### Code reviews

All submissions, including submissions by project members, require review. We use GitHub pull requests for this purpose. Consult GitHub Help for more information on using pull requests.