%define PACKAGE_NAME postgis-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

%define REQ_PROJ_VER 4.5.0

Summary:      PostGIS extention to the PostgreSQL databse
Name:         %PACKAGE_NAME
Version:      1.1.7
Release:      1
License:      GPL
Group:        Development/Libraries/C and C++
Source0:      postgis-%{version}.tar.gz
# Patch0:        expat-DESTDIR.patch
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://postgis.refractions.net/
AutoReqProv: no
 
BuildRequires: proj-ge-devel >= %REQ_PROJ_VER
Requires: proj-ge >= %REQ_PROJ_VER
BuildRequires: geos-ge-devel
Requires: geos-ge
BuildRequires: postgresql-ge-devel
Requires: postgresql-ge
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
PostGIS extention to the PostgreSQL databse

%prep
%setup -q -n postgis-%{version}
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
# grhat's ssl needs kerberos
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure \
    --prefix=%{_prefix} \
    --mandir=%{_mandir} \
    --with-pgsql=%{_prefix}/bin/pg_config \
    --with-proj=%{_prefix} \
    --with-geos=%{_prefix}/bin/geos-config
make -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install
rm $RPM_BUILD_ROOT%{_prefix}/lib/*.so

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc COPYING CREDITS README.postgis
%{_prefix}/lib/*.so.*
%{_prefix}/bin/*
%{_prefix}/share/*.sql
