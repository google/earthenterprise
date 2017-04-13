**Earth Enterprise** is the open source release of Google Earth Enterprise, a geospatial application which provides the ability to build and host custom 3D globes and 2D maps.  Earth Enterprise does not provide a private version of Google imagery that's currently available in Google Maps or Earth.

The application suite consists of three core components:
* Fusion - imports and 'fuses' imagery, vector and terrain source data in to a single flyable 3D globe or 2D map.
* Server - Apache or Tornado-based server which hosts the private globes built by Fusion.
* Client - the Google Earth Enterprise Client (EC) and Google Maps Javascript API V3 used to view 3D globes and 2D maps, respectively.

![flow](https://lh3.googleusercontent.com/ZGQH04lc2mYmw1JEx0Jvwiardw5H6cwrmRhSj75pSKF6r1FRwwYUBUIBnTE6n5uY071XV7__mmVDKdV6B1tEpUQwFNYnt1HBfxiz3Hrqbw99HUFQKVFnht11EkPz70xCtuhFlCi3)

### Building
Earth Enterprise Fusion & Server currently run on 64-bit versions of the following operating systems:

* Red Hat Enterprise Linux 7, including the most recent security patches
* Ubuntu 14.04 LTS

Refer to the [wiki](https://github.com/google/earthenterprise/wiki/Build-Instructions) for instructions on building from source on one of these platforms.

### Installation
There is currently no official installer for Earth Enterprise Fusion & Server.  For details and workarounds, refer to the [FAQ](https://github.com/google/earthenterprise/wiki/Frequently-Asked-Questions#ive-built-gee-fusion-and-server-now-how-do-i-install-them).