%define PACKAGE_NAME openldap-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _etcdir /etc/opt/google
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}


Summary:      OpenLDAP client utilities
Name:         %PACKAGE_NAME
Version:      2.4.11
Release:      0
License:      BSD
Group:        Productivity/Networking/LDAP/Clients
Source:       openldap-%{version}.tgz
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://www.openldap.org/
AutoReqProv: no

BuildRequires: openssl-ge-devel >= 0.9.8
BuildConflicts: openssl-devel

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
This package contains the OpenLDAP client utilities

%package devel
Summary:      Libraries, Header Files and Documentation for OpenLDAP
Group:        Development/Libraries/C and C++
Requires: %{name} = %{version}
Requires: openssl-ge-devel >= 0.9.8
AutoReqProv: no

%description devel
This package contains the Libraries, Header Files and Documentation for
OpenLDAP



%prep
%setup -q -n openldap-%{version}

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
export CPPFLAGS='-I%{_prefix}/include'
export LDFLAGS='-L%{_prefix}/lib'
if [ -f /etc/sysconfig/grhat ] ; then
#	export CPPFLAGS="$CPPFLAGS -I/usr/kerberos/include"
	EXTRAFLAGS='--without-cyrus-sasl'
fi
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' \
./configure $EXTRAFLAGS \
    --prefix=%_prefix \
    --mandir=%_mandir \
    --sysconfdir=%_etcdir \
    --disable-dependency-tracking \
    --disable-slapd \
    --disable-slurpd \
    --with-threads \
    --with-tls
make -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install
rm -rf $RPM_BUILD_ROOT%{_mandir}/man8
rm -rf $RPM_BUILD_ROOT%{_mandir}/man5/slap*
rm -rf $RPM_BUILD_ROOT%{_prefix}/include/slap*.h


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc ANNOUNCEMENT CHANGES COPYRIGHT LICENSE README
%{_prefix}/bin
%{_etcdir}/openldap
%{_prefix}/lib/*.so.*
%{_mandir}/man1/*
%{_mandir}/man5/*

%files devel
%defattr(-, root, root)
%{_mandir}/man3/*
%{_prefix}/include/*
%{_prefix}/lib/*.a
%{_prefix}/lib/*.la
%{_prefix}/lib/*.so
