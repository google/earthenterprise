%define PACKAGE_NAME libmng-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary: A library of functions for manipulating reading, displaying, writing and examining Multiple-Image Network Graphics.
Name: %PACKAGE_NAME
Version: 1.0.10
Release: 1
License: distributable
Group: System Environment/Libraries
Source0: libmng-%{version}.tar.gz
#Patch0: libmng-config.patch
URL: http://www.libmng.com/
BuildRoot: %{_tmppath}/%{name}-root
AutoReqProv: no

Requires: zlib libjpeg

BuildRequires: zlib-devel zlib libjpeg-devel libjpeg
BuildRequires: gcc-ge >= 4.1.1
Requires:      gcc-ge-runtime >= 4.1.1
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
The %PACKAGE_NAME package contains a specially packaged version of the
libmng library of functions for manipulating MNG (Multiple-Image Network
Graphics) image format files. MNG is the animation extension to the popular
PNG image-format.
The %PACKAGE_NAME package should be installed if you need to
manipulate MNG format image files.

%PACKAGE_NAME differs from the standard libmng only in the location of its
files. Because Google Earth products need to work on a number of different
platforms with different native versions of libmng, Google has decided to
install it's own version under %_prefix.


%package devel
Summary: Development tools for the %PACKAGE_NAME library.
Group: Development/Libraries
Requires: %PACKAGE_NAME = %{version}
AutoReqProv: no

%description devel
This package contains the header files and static libraries for
developing programs which will manipulate MNG format image files
using the %PACKAGE_NAME library.

If you need to develop programs which will manipulate MNG format
image files, you should install this package.

%prep
%setup -q -n libmng-%{version}
# %patch0 -p1 -b .config

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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./unmaintained/autogen.sh --prefix=%_prefix
make -j $NRPROC

%install
[ "$RPM_BUILD_DIR" ] && rm -fr $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
DESTDIR=$RPM_BUILD_ROOT make mandir=%_mandir install
rm -rf $RPM_BUILD_ROOT/%_prefix/share/doc/libmng-%{version}

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc LICENSE
%doc README
%doc CHANGES
%{_bindir}/*
%{_libdir}/libmng.so*
%doc %{_mandir}/man1/*

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libmng.a
%{_libdir}/libmng.la
%doc %{_mandir}/man3/*
