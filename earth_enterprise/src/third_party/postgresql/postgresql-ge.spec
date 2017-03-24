%define PACKAGE_NAME postgresql-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}


Summary:      PostgreSQL database
Name:         %PACKAGE_NAME
Version:      8.1.8
Release:      4
License:      BSD
Group:        Development/Libraries/C and C++
Source0:       postgresql-base-%{version}.tar.bz2
Source1:       postgresql-opt-%{version}.tar.bz2
# Patch0:        expat-DESTDIR.patch
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://www.postgresql.org/
AutoReqProv: no
 
BuildRequires: zlib-devel
Requires: zlib
BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1
BuildRequires: libtool
BuildConflicts: libtool-32bit
Requires: python
BuildRequires: python-devel

BuildRequires: openssl-ge-devel >= 0.9.8
BuildConflicts: openssl-devel

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
The worlds most advanced open source database.

%package devel
Summary:      PostgeSQL - Development files
Group:        Development/Libraries/C and C++
License:      BSD
Requires: %{name} = %{version}
Requires: openssl-ge-devel >= 0.9.8
Requires: zlib-devel
AutoReqProv: no

%description devel
postgresql development files



%prep
%setup -q -n postgresql-%{version}
%setup -q -T -D -b 1 -n postgresql-%{version}
# %patch0 -p1

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
export INCLUDES='-I%{_prefix}/include'
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure \
    --prefix=%{_prefix} \
    --mandir=%{_mandir} \
    --disable-rpath \
    --enable-thread-safety \
    --with-openssl \
    --with-python \
    --without-perl \
    --without-readline \
    --without-pam \
    --without-krb5
make -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install

# This is where the data and log files live
mkdir -p $RPM_BUILD_ROOT/var/opt/google/pgsql/logs

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc COPYRIGHT HISTORY README 
%{_prefix}/lib/*.so.*
%{_prefix}/lib/postgresql/*
%{_mandir}/man1/*
%{_prefix}/share/postgresql/*
%{_prefix}/bin/*
%attr(0700, gepguser, gegroup) /var/opt/google/pgsql

%files devel
%defattr(-, root, root)
%{_prefix}/include/*
%{_prefix}/lib/*.a
%{_prefix}/lib/*.so
%{_mandir}/man7/*
