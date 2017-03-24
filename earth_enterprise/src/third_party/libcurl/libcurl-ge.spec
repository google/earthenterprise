%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define _libdir %_prefix/lib
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Name:         libcurl-ge
License:      MIT
Group:        System/Libraries
Autoreqprov:  no
Summary:      A library for HTTP access
Version:      7.19.3
Release:      0
Source:       curl-%{version}.tar.gz
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://curl.haxx.se/libcurl

%description
curl HTTP library needed by Fusion

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

%package devel
Summary:      libcurl development files
Group:        Development/Libraries/C and C++
Autoreqprov:  no
Requires:     %{name} = %{version}

%description devel
libcurl development files


%prep
%setup -q -n curl-%{version}

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
export LDFLAGS=-L%{_libdir}
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure \
    --prefix=%{_prefix} \
    --mandir=%_mandir \
    --libdir=%{_libdir} \
    --without-ssl \
    --without-ca-bundle \
    --without-zlib \
    --without-libidn \
    --without-krb4 \
    --without-gnutls
make -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT%{_libdir}/libcurl.a
rm -f $RPM_BUILD_ROOT%{_libdir}/libcurl.la
rm -r $RPM_BUILD_ROOT%{_prefix}/bin/curl
rm -r $RPM_BUILD_ROOT%{_prefix}/bin/curl-config
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/pkgconfig/libcurl.pc
rm -r $RPM_BUILD_ROOT%{_mandir}/man1


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc CHANGES COPYING README RELEASE-NOTES
%{_libdir}/lib*.so.*

%files devel
%defattr(-, root, root)
%{_libdir}/lib*.so
%{_prefix}/include/*
%{_prefix}/share/man/man3/*

