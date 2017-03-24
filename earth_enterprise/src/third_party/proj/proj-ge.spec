%define PACKAGE_NAME proj-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary: Cartographic projection software
Name: %PACKAGE_NAME
Version: 4.6.1
Release: 0
Source0: proj-%{version}.tar.gz
Source1: proj-datumgrid-1.3.zip
#Patch0:  proj-OSGB1936.patch
License: MIT License, Copyright (c) 2000, Frank Warmerdam
Group: Applications/GIS
Conflicts: libproj0
Conflicts: libproj0-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1
BuildRequires: libtool
BuildConflicts: libtool-32bit

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
Cartographic projection software.

%package devel
Summary: Development tools for programs which will use the proj.4 library
Group: Development/Libraries
Requires: %{name} = %{version}
AutoReqProv: no

%description devel
This package contains the header files and static libraries for
developing programs using the proj.4 library.


%prep
%setup -n proj-%{version}
%setup -T -D -a 1 -n proj-%{version}/nad
%setup -T -D -n proj-%{version}
#%patch0 -p1

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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure --prefix=%{_prefix}
make -j $NRPROC

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
DESTDIR=%{buildroot} make mandir=%_mandir install

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_bindir}/*
%{_libdir}/*.so*
%{_datadir}/proj/
%doc %{_mandir}/man1/*
%doc AUTHORS COPYING ChangeLog README


%files devel
%defattr(-,root,root)
%{_libdir}/*.a
%{_libdir}/*.la
%{_includedir}/*
%doc %{_mandir}/man3/*
