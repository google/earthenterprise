%define PACKAGE_NAME libgeotiff-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

%define REQ_PROJ_VER 4.5.0
%define REQ_LIBTIFF_VER 3.8.2

Summary: A library of functions for manipulating geo referenced TIFF images.
Name: %PACKAGE_NAME
Version: 1.2.3
Release: 4
License: Public Domain / MIT
Group: Sciences/Geosciences
Source0: libgeotiff-%{version}.tar.gz
Patch0:  libgeotiff-fixprefix.patch
URL: http://www.remotesensing.com/geotiff/geotiff.html
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1

Requires: libtiff-ge >= %REQ_LIBTIFF_VER
Requires: proj-ge >= %REQ_PROJ_VER

BuildRequires: libtiff-ge-devel >= %REQ_LIBTIFF_VER
BuildRequires: proj-ge-devel >= %REQ_PROJ_VER

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -march=i686 -O2
%define ldrunpath %_prefix/lib
%endif

%ifarch x86_64
%define optflags -O2 -fPIC
%define ldrunpath %_prefix/lib64:%_prefix/lib
%endif

%description
This library is designed to permit the extraction and parsing of the
"GeoTIFF" Key directories, as well as definition and installation
of GeoTIFF keys in new files. For more information about GeoTIFF
specifications, projection codes and use, see the WWW web page at:
   http://www.remotesensing.com/geotiff/geotiff.html
or at:
   http://www.geotiff.org/

%package devel
Summary: Cartographic software - Development files
Group: Sciences/Geosciences
License: Public Domain / MIT
Requires: %{name} = %{version}
Requires: libtiff-ge-devel >= %REQ_LIBTIFF_VER
Requires: proj-ge-devel >= %REQ_PROJ_VER
AutoReqProv: no

%description devel
libgeotiff development files.

%prep
%setup -q -n libgeotiff-%{version}
%patch0 -p1

%build
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' \
# tarball has configure slightly out of date
# this prevents it from automatically remaking and being invoked again
# with the wrong args
touch configure

./configure \
    --prefix=%_prefix \
    --mandir=%{_mandir} \
    --with-libtiff=%{_prefix} \
    --with-proj=%{_prefix}
# can't deal with -j :-(
make
rm docs/api/.cvsignore

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install
chmod 644 $RPM_BUILD_ROOT%{_includedir}/*

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files 
%defattr(-,root,root)
%{_bindir}/*
%{_libdir}/*.so*
%{_prefix}/share/epsg_csv/*.csv
%doc docs/*
%doc README LICENSE ChangeLog

%files devel
%defattr(-,root,root)
%{_libdir}/*.a
%{_includedir}/*
