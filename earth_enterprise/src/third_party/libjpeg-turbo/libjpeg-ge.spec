%define PACKAGE_NAME libjpeg-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary: A library for manipulating JPEG image format files.
Name: %PACKAGE_NAME
Version: 1.5.0
Release: 1
License: distributable
Group: System Environment/Libraries
Source0: libjpeg-turbo-%{version}.tar.gz
#Patch0: libjpeg-config.patch
URL:  http://www.ijg.org/
BuildRoot: %{_tmppath}/%{name}-%{version}-root
AutoReqProv: no

Requires: 

BuildRequires: gcc-ge >= 4.1.1
Requires:      gcc-ge-runtime >= 4.1.1
BuildRequires: libtool
BuildConflicts: libtool-32bit

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -march=i686 -O3 -msse2
%define ldrunpath %_prefix/lib
%endif

%ifarch x86_64
%define optflags -O3 -msse2
%define ldrunpath %_prefix/lib64:%_prefix/lib
%endif

%description
The %PACKAGE_NAME package contains a library of functions for manipulating
JPEG images, as well as simple client programs for accessing the
libjpeg functions.  Libjpeg client programs include cjpeg, djpeg,
jpegtran, rdjpgcom and wrjpgcom.  Cjpeg compresses an image file into
JPEG format.  Djpeg decompresses a JPEG file into a regular image
file. Jpegtran can perform various useful transformations on JPEG
files. Rdjpgcom displays any text comments included in a JPEG file.
Wrjpgcom inserts text comments into a JPEG file.
The %PACKAGE_NAME package should be installed if you need to manipulate
JPEG format image files.


%PACKAGE_NAME differs from the standard libjpeg only in the location of its
files. Because Google Earth products need to work on a number of different
platforms with different native versions of libjpeg, Google has decided to
install it's own version under %_prefix.


%package devel
Summary: Development tools for the %PACKAGE_NAME library.
Group: Development/Libraries
Requires: %PACKAGE_NAME = %{version}
AutoReqProv: no

%description devel
This package contains the header files and static libraries for
developing programs which will manipulate JPEG format image files
using the %PACKAGE_NAME library.

If you need to develop programs which will manipulate JPEG format
image files, you should install this package.  You'll also need to
install the %PACKAGE_NAME package.

%prep
%setup -q -n jpeg-%{version}
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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure --prefix=%_prefix
make -j $NRPROC

%install
[ "$RPM_BUILD_DIR" ] && rm -fr $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
DESTDIR=$RPM_BUILD_ROOT make mandir=%_mandir install
rm -rf $RPM_BUILD_ROOT/%_prefix/share/doc/%{PACKAGE_NAME}-%{version}

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%{_bindir}/*
%{_libdir}/libjpeg.so*
%doc %{_mandir}/man1/*
%doc change.log README

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libjpeg.a
%{_libdir}/libjpeg.la
%doc %{_mandir}/man3/*
