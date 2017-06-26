%define PACKAGE_NAME gdal-ge
%define PACKAGE_VERSION 2.1.2
%define KAKADU_SOURCEDIR v5_2_6-00418C
%define KAKADU_ZIPFILE %KAKADU_SOURCEDIR.zip
%define OPENJPEG_VERSION 20100

%define MRSID_VERSION 8.5.0.3422
%define MRSID_PLATFORM %(eval "%_topdir/SOURCES/GDALMrSIDConfig.pl platform")
%define with_mrsid %(eval "%_topdir/SOURCES/GDALMrSIDConfig.pl  --version=%MRSID_VERSION withmrsid")

%define ECW_VERSION 5.0
%define PACKAGE_URL http://www.remotesensing.org/gdal
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}
%define ECW_DIR libecwj2-%ECW_VERSION

%define REQ_TIFF_VER 4.0.3
%define REQ_GEOTIFF_VER 1.2.3
%define REQ_PROJ_VER 4.5.0
%define REQ_OGDI_VER 3.1.5
%define REQ_XERCES_VER 2.7.0
%define REQ_GEOS_VER 2.2.3

Summary: GDAL - a translator library for raster geospatial data formats
Name: %PACKAGE_NAME
Version: %PACKAGE_VERSION
Release: 0
Source0: gdal-%{version}.tar.gz
Source1: %KAKADU_ZIPFILE
Source2: Geo_DSDK-%MRSID_VERSION.%MRSID_PLATFORM.tar.gz
Source3: GDALMrSIDConfig.pl
# We don't have a license for ECW. Would like this someday.
#T %define ECW_ZIPFILE libecwj2-3.3-RC2-2006-02-08.zip
#T Source5: %ECW_ZIPFILE

# Needed to build 64 bit kakadu (verified as of 1.6) --> no longer needed as of 1.10
# Patch2: kakadu-fPIC.patch
# Needed to fix 2 datums (verified as of 1.6)
# Patch9: gdal-GBdatum.patch --> no longer needed as of 1.10.0
# Google patch to improve gdal warp cubic interpolation performance (as of Fusion 3.0.1).
Patch11: gdal-performance.patch
# Google patch to fix build errors of hard linking to ~/packages/BUILD....
Patch12: gdal-apps-make.patch
License: X/MIT license; Copyright by Frank Warmerdam
Group: Applications/GIS
URL: %PACKAGE_URL
Vendor: Intevation GmbH <http://intevation.net>
Distribution: FreeGIS CD
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1

# GDAL doesn't compile clean if another version is already installed
BuildConflicts: gdal-ge
BuildConflicts: gdal-ge-devel

Requires: libtiff-ge    >= %REQ_TIFF_VER
Requires: libgeotiff-ge >= %REQ_GEOTIFF_VER
Requires: proj-ge       >= %REQ_PROJ_VER
Requires: ogdi-ge       >= %REQ_OGDI_VER
Requires: xerces-c-ge   >= %REQ_XERCES_VER
Requires: geos-ge       >= %REQ_GEOS_VER

BuildRequires: libtiff-ge-devel    >= %REQ_TIFF_VER
BuildRequires: libgeotiff-ge-devel >= %REQ_GEOTIFF_VER
BuildRequires: proj-ge-devel       >= %REQ_PROJ_VER
BuildRequires: ogdi-ge-devel       >= %REQ_OGDI_VER
BuildRequires: xerces-c-ge-devel   >= %REQ_XERCES_VER
BuildRequires: geos-ge-devel       >= %REQ_GEOS_VER

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -march=i686 -O2
%define with_ecw --with-ecw=`pwd`/%ECW_DIR
%define ldrunpath %_prefix/lib:/usr/lib
%define ldflags -L%_prefix/lib -L/usr/lib
%endif

%ifarch x86_64
%define optflags -O2
%define with_ecw --with-ecw=`pwd`/%ECW_DIR
%define ldrunpath %_prefix/lib64:%_prefix/lib:/usr/lib64:/lib64:/usr/lib
%define ldflags -L%_prefix/lib64 -L%_prefix/lib -L/usr/lib64 -L/lib64 -L/usr/lib
%endif

%description
GDAL is a translator library for raster geospatial data formats. As a library,
it presents a single abstract data model to the calling application for all supported
formats. It also comes  with a variety of useful commandline utilities for data translation
and processing.

%package devel
Summary: GDAL - Development files
Group: Libraries/GIS
License: X/MIT license; Copyright by Frank Warmerdam
Requires: %PACKAGE_NAME = %{version}
Requires: libtiff-ge-devel    >= %REQ_TIFF_VER
Requires: libgeotiff-ge-devel >= %REQ_GEOTIFF_VER
Requires: proj-ge-devel       >= %REQ_PROJ_VER
Requires: ogdi-ge-devel       >= %REQ_OGDI_VER
Requires: xerces-c-ge-devel   >= %REQ_XERCES_VER
Requires: geos-ge-devel     >= %REQ_GEOS_VER
AutoReqProv: no

%description devel
GDAL development files

%prep
%setup -q -n gdal-%{version}
unzip -q $RPM_SOURCE_DIR/%KAKADU_ZIPFILE
%ifarch x86_64
cd %KAKADU_SOURCEDIR
%patch2 -p2
cd ..
%endif
if [ x%{with_mrsid} != x--with-mrsid=no ] ; then
    tar xzf $RPM_SOURCE_DIR/Geo_DSDK-%MRSID_VERSION.%MRSID_PLATFORM.tar.gz
fi
#T unzip -q $RPM_SOURCE_DIR/%ECW_ZIPFILE
%patch9 -p1
%patch11 -p1
%patch12 -p1

%build
if [ -x /usr/bin/getconf ] ; then
  NRPROC=$(/usr/bin/getconf _NPROCESSORS_ONLN)
   if [ $NRPROC -eq 0 ] ; then
    NRPROC=1
  fi
else
  NRPROC=1
fi
NRPROC=$(($NRPROC*2))
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath


# prebuild Kakadu library
make -j $NRPROC -C %KAKADU_SOURCEDIR/coresys/make -f Makefile-Linux-x86-64-gcc
mkdir -p %KAKADU_SOURCEDIR/java/kdu_jni
make -j $NRPROC -C %KAKADU_SOURCEDIR/apps/make -f Makefile-Linux-x86-64-gcc
ln %KAKADU_SOURCEDIR/lib/Linux-x86-64-gcc/libkdu.a %KAKADU_SOURCEDIR/lib

#T # prebuild ECW static library
#T if [ `uname -m` != ia64 ] ; then
#T     make -j $NRPROC -C %ECW_DIR/Source/NCSBuildQmake -f Makefile-linux-static
#T fi

# disable libtool, it breaks lots and lots of things
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' LDFLAGS='%{ldflags}' \
./configure \
    --prefix=%{_prefix}  \
    --with-gnu-ld \
    --without-libtool \
    --disable-static \
    --with-threads \
    --with-ogdi=%{_prefix} \
    --with-libtiff=%{_prefix} \
    --with-geotiff=%{_prefix} \
    --with-xerces=%{_prefix} \
    --with-geos=%{_prefix}/bin/geos-config \
    --with-kakadu=`pwd`/%KAKADU_SOURCEDIR \
    %{with_mrsid} \
    --without-python \
    --without-pg \
    --without-jasper \
    --without-odbc \
    --without-mysql \
    --without-sqlite \
    --without-jp2mrsid \
    --with-grass=no \
    --with-oci=no \
    --with-gif=no \
\
    --with-ecw=no \
    --with-curl=no

#T     %{with_ecw} \

# build GDAL itself
make -j $NRPROC

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make prefix=%{buildroot}%{_prefix} install
install -m 0444 frmts/vrt/vrtdataset.h %{buildroot}%{_includedir}/vrtdataset.h
rm -rf %{buildroot}%{_prefix}/man

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_bindir}/*
%{_datadir}/gdal/
%doc VERSION NEWS
%{_libdir}/*.so*

%files devel
%defattr(-,root,root)
%{_includedir}/*
