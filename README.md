# Earth Enterprise [![Chat on Slack](https://img.shields.io/badge/chat-on%20slack-ff69b4.svg)](http://slack.opengee.org)

Earth Enterprise is the open source release of Google Earth Enterprise, a geospatial application which provides the ability to build and host custom 3D globes and 2D maps.  Earth Enterprise does not provide a private version of Google imagery that's currently available in Google Maps or Earth.

The application suite consists of three core components:
* Fusion - imports and 'fuses' imagery, vector and terrain source data in to a single flyable 3D globe or 2D map.
* Server - Apache or Tornado-based server which hosts the private globes built by Fusion.
* Client - the Google Earth Enterprise Client (EC) and Google Maps Javascript API V3 used to view 3D globes and 2D maps, respectively.

![flow](https://lh3.googleusercontent.com/ZGQH04lc2mYmw1JEx0Jvwiardw5H6cwrmRhSj75pSKF6r1FRwwYUBUIBnTE6n5uY071XV7__mmVDKdV6B1tEpUQwFNYnt1HBfxiz3Hrqbw99HUFQKVFnht11EkPz70xCtuhFlCi3)

### Building
Earth Enterprise Fusion & Server currently run on 64-bit versions of the following operating systems:

* CentOS 7
* Red Hat Enterprise Linux 7
* Ubuntu 14.04 LTS
* Ubuntu 16.04 LTS

Refer to the [wiki](https://github.com/google/earthenterprise/wiki/Build-Instructions) for instructions on building from source on one of these platforms.

### Installation
Refer to the [Install Instructions](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server) for instructions on installing Fusion or Earth Server.  Please note that you must have a successful build of the source before proceeding with the install.
