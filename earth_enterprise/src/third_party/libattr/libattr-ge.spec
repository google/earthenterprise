%define PACKAGE_NAME libattr-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %{_prefix}/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary: A dynamic library for filesystem extended attribute support.
Name: %PACKAGE_NAME
Version: 2.4.47
Release: 1
License: distributable
Group: System/Filesystems
Source0: attr-%{version}.src.tar.gz
#Patch0: libattr-config.patch
URL:  http://download.savannah.gnu.org/releases/attr/
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
%define optflags -march=i686 -O2
%define ldrunpath %_prefix/lib
%endif

%ifarch x86_64
%define optflags -O2
%define ldrunpath %_prefix/lib64:%_prefix/lib
%endif

%description
The %PACKAGE_NAME package contains the libattr.so dynamic library, which 
contains the extended attribute library functions.

%PACKAGE_NAME differs from the standard libattr only in the location of its
files. Because Google Earth products need to work on a number of different
platforms with different native versions of libattr, Google has decided to
install it's own version under %_prefix.


%package devel
Summary: Development tools for the %PACKAGE_NAME library.
Group: Development/Libraries
Requires: %PACKAGE_NAME = %{version}
AutoReqProv: no

%description devel
This package contains the libraries and header files needed to develop
programs which make use of extended attributes. For Linux programs, the
documented system call API is the recommended interface.

%prep
%setup -q -n attr-%{version}
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
DESTDIR=$RPM_BUILD_ROOT make mandir=%_mandir install install-dev install-lib
rm -rf $RPM_BUILD_ROOT/%_prefix/share/doc/%{PACKAGE_NAME}-%{version}

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%{_bindir}/*
%{_libdir}/libattr.so*
%doc %{_mandir}/man1/*
%doc %{_mandir}/man2/*
%doc %{_mandir}/man3/*
%doc %{_mandir}/man5/*
%doc COPYING CHANGES README VERSION

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libattr.a
%{_libdir}/libattr.la
%doc %{_mandir}/man1/*
%doc %{_mandir}/man2/*
%doc %{_mandir}/man3/*
%doc %{_mandir}/man5/*
