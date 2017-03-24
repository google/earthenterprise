%define PACKAGE_NAME ogdi-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

%define REQ_PROJ_VER 4.5.0
%define REQ_EXPAT_VER 2.0.0


Summary: OGDI (Open Geographic Datastore Interface)
Name: %PACKAGE_NAME
Version: 3.1.5
Release: 6
License: distributable
Group: Sciences/Geosciences
Source0: ogdi-%{version}.tar.gz
Patch0: ogdi-topdir.patch
Patch1: ogdi-errmsg.patch
#Patch2: ogdi-proj.patch
URL: http://ogdi.sourceforge.net/
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1

Requires: proj-ge >= %REQ_PROJ_VER
BuildRequires: proj-ge-devel >= %REQ_PROJ_VER

Requires: expat-ge >= %REQ_EXPAT_VER
BuildRequires: expat-ge-devel >= %REQ_EXPAT_VER

Requires: zlib
BuildRequires: zlib-devel

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -march=i686 -O2
%define ldrunpath %_prefix/lib
%endif

%ifarch x86_64
%define optflags -O2
%define ldrunpath %_prefix/lib64:%_prefix/lib
%endif



%description
OGDI is the Open Geographic Datastore Interface. OGDI is an
application programming interface (API) that uses a standardized access
methods to work in conjunction with GIS software packages (the application)
and various geospatial data products. OGDI uses a client/server architecture
to facilitate the dissemination of geospatial data products over any TCP/IP
network, and a driver-oriented approach to facilitate access to several
geospatial data products/formats.

The Open Source OGDI core technology, and the OGDI interface standard was
originally developed largely by Global Geomatics under contract to the
Canadian Department of National Defence. From 2000 to 2002 it was
maintained by the Information Interoperability Institute which is no longer
in operation. Now it is maintained on a volunteer basis, primarily by Frank
Warmerdam.

%package devel
Summary: OGDI (Open Geographic Datastore Interface) - Development files
License: distributable
Group: Sciences/Geosciences
Requires: %{name} = %{version}
Requires: proj-ge-devel >= %REQ_PROJ_VER
Requires: expat-ge-devel >= %REQ_EXPAT_VER
AutoReqProv: no

%description devel
OGDI (Open Geographic Datastore Interface) - Development files


%prep
%setup -q -n ogdi-%{version}
%patch0 -p1 -b .topdir
%patch1 -p1 -b .errmsg
#%patch2 -p1

%build
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
CFLAGS='%{optflags} -fPIC' CXXFLAGS='%{optflags} -fPIC' \
./configure \
    --prefix=%{_prefix} \
    --with-proj=%{_prefix} \
    --with-expat=%{_prefix} \
    --with-zlib
make OPTIMIZATION='%{optflags}'

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make prefix=$RPM_BUILD_ROOT/%{_prefix} install
rm $RPM_BUILD_ROOT/%{_prefix}/bin/example*

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi


%files
%defattr(-,root,root)
%doc ChangeLog LICENSE NEWS README VERSION
%{_bindir}/*
%{_libdir}/*

%files devel
%{_includedir}/*
