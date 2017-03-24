%define openssl_ver 0.9.8
%define librev l

%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define buildpath %_prefix/bin:/bin:/usr/bin
%define ssl_root %{_prefix}
%define varopenssldir /var/opt/google/openssl
%define debug_package %{nil}

Summary: Secure Sockets Layer and cryptography libraries and tools
Name: openssl-ge
Version: %{openssl_ver}%{librev}
Release: 0
Source0: openssl-%{version}.tar.gz
License: Freely distributable
Group: System Environment/Libraries
URL: http://www.openssl.org/
BuildRoot: %{_tmppath}/%{name}-%{version}-root
AutoReqProv: no

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define ldrunpath %_prefix/lib
%define buildplatform linux-elf
%endif

%ifarch x86_64
%define ldrunpath %_prefix/lib64:%_prefix/lib
%define buildplatform linux-x86_64
%endif



%description
The OpenSSL Project is a collaborative effort to develop a robust,
commercial-grade, fully featured, and Open Source toolkit implementing the
Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS v1)
protocols as well as a full-strength general purpose cryptography library.
The project is managed by a worldwide community of volunteers that use the
Internet to communicate, plan, and develop the OpenSSL tookit and its related
documentation.

OpenSSL is based on the excellent SSLeay library developed from Eric A.
Young and Tim J. Hudson.  The OpenSSL toolkit is licensed under an
Apache-style licence, which basically means that you are free to get and
use it for commercial and non-commercial purposes.

This package contains the base OpenSSL cryptography and SSL/TLS
libraries and tools.



%package devel
Summary: Secure Sockets Layer and cryptography static libraries and headers
Group: Development/Libraries
Requires: openssl-ge
AutoReqProv: no
%description devel
Development pieces for OpenSSL






%prep
%setup -q -n openssl-%{version}

%build
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
export LDFLAGS='-shared-libgcc -L%{ldrunpath}'

perl util/perlpath.pl /usr/bin/perl

./Configure \
    -DSSL_ALLOW_ADH \
    --prefix=%{ssl_root} \
    --openssldir=%{varopenssldir} \
    %{buildplatform} \
    shared
# openssl's makefile cannot handle -j NRPROC
make


%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
make INSTALL_PREFIX=$RPM_BUILD_ROOT install_sw
rm -rf $RPM_BUILD_ROOT%{ssl_root}/lib/pkgconfig
rm -rf $RPM_BUILD_ROOT%{ssl_root}/lib/fips_premain.*
rm -rf $RPM_BUILD_ROOT%{ssl_root}/lib/engines


%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(0644,root,root,0755)
%doc CHANGES
%doc CHANGES.SSLeay
%doc LICENSE
%doc NEWS
%doc README

%attr(0755,root,root) %{ssl_root}/bin
%attr(0755,root,root) %{ssl_root}/lib/*.so.*
%attr(0755,root,root) %{varopenssldir}/misc/*

%config %attr(0644,root,root) %{varopenssldir}/openssl.cnf
%dir %attr(0755,root,root) %{varopenssldir}/certs
%dir %attr(0755,root,root) %{varopenssldir}/misc
%dir %attr(0750,root,root) %{varopenssldir}/private

%files devel
%defattr(0644,root,root,0755)
%attr(0644,root,root) %{ssl_root}/lib/*.a
%attr(0755,root,root) %{ssl_root}/lib/*.so
%attr(0644,root,root) %{ssl_root}/include/openssl/*

