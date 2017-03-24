%define PACKAGE_NAME libtiff-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary: A library of functions for manipulating TIFF format image files.
Name: %PACKAGE_NAME
Version: 4.0.3
Release: 2
License: distributable
Group: System Environment/Libraries
Source0: tiff-%{version}.tar.gz
URL: http://www.libtiff.org/
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
libtiff library of functions for manipulating TIFF (Tagged Image File
Format) image format files.  TIFF is a widely used file format for bitmapped
images.  TIFF files usually end in the .tif extension and they are often
quite large. The %PACKAGE_NAME package should be installed if you need to
manipulate TIFF format image files.

%PACKAGE_NAME differs from the standard libtiff only in the location of its
files. Because Google Earth products need to work on a number of different
platforms with different native versions of libtiff, Google has decided to
install it's own version under %_prefix.


%package devel
Summary: Development tools for the %PACKAGE_NAME library.
Group: Development/Libraries
Requires: %PACKAGE_NAME = %{version}
AutoReqProv: no

%description devel
This package contains the header files and static libraries for
developing programs which will manipulate TIFF format image files
using the %PACKAGE_NAME library.

If you need to develop programs which will manipulate TIFF format
image files, you should install this package.  You'll also need to
install the %PACKAGE_NAME package.

%prep
%setup -q -n tiff-%{version}
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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure --prefix=%_prefix --without-x --without-gl --without-glut --disable-cxx
make -j $NRPROC

%install
[ "$RPM_BUILD_DIR" ] && rm -fr $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
DESTDIR=$RPM_BUILD_ROOT make mandir=%_mandir install
rm -rf $RPM_BUILD_ROOT/%_prefix/share/doc/tiff-%{version}

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc COPYRIGHT
%doc README
%doc VERSION
%{_bindir}/*
%{_libdir}/libtiff.so*
%doc %{_mandir}/man1/*

%files devel
%defattr(-,root,root)
%doc ChangeLog
%doc RELEASE-DATE
%doc TODO
%doc html
%{_includedir}/*
%{_libdir}/libtiff.a
%{_libdir}/libtiff.la
%doc %{_mandir}/man3/*
