%define _prefix /opt/google
%define _mandir %_prefix/share/man
%define _infodir %_prefix/share/info
%define _libdir %_prefix/lib
%define debug_package %{nil}

Summary: GNU Compiler - Packaged by Google for a consistent build environment
Name: gcc-ge
Version: 4.1.1
Release: 4
Source0: gcc-core-%{version}.tar.bz2
Source1: gcc-g++-%{version}.tar.bz2
License: GPL
Group: Development/Languages/C and C++
URL: http://gcc.gnu.org
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: binutils-ge >= 2.16.91.0.7

ExclusiveArch: i686
ExclusiveArch: x86_64

%define have_lib64 0

%ifarch i686
%endif

%ifarch x86_64
%define have_lib64 1
%endif

%description
C and C++ compiler.
This version has been compiled by Google.


%package runtime
Summary:      C compiler runtime library
Group:        System/Base
Autoreqprov:  on

%description runtime
Runtime libraries needed for programs linked with gcc-ge.




%prep
%setup -q -c
%setup -q -D -T -a 1


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
mkdir build
cd build
../gcc-%{version}/configure \
    --prefix=%{_prefix}  \
    --mandir=%_mandir \
    --infodir=%_infodir \
    --enable-threads=posix \
    --with-system-zlib \
    --enable-__cxa_atexit \
    --disable-checking \
    --enable-shared \
    --without-system-libunwind \
    --disable-libmudflap \
    --disable-libssp \
    --without-mpfr \
    --without-gmp \
    --enable-languages=c,c++

make bootstrap-lean BOOT_CLFAGS="-O2" -j $NRPROC

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
cd build
make install DESTDIR=$RPM_BUILD_ROOT
rm $RPM_BUILD_ROOT/%{_libdir}/libiberty.a
rm -f $RPM_BUILD_ROOT/%{_prefix}/share/locale/*/LC_MESSAGES/libstdc++.mo
%if %{have_lib64}
rm -f $RPM_BUILD_ROOT/%{_prefix}/lib/lib*
%endif

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_prefix}/bin
%{_prefix}/include
%{_prefix}/share/info
%{_prefix}/libexec
%{_prefix}/share/man
%{_prefix}/share/locale/*/LC_MESSAGES/cpplib.mo
%{_prefix}/share/locale/*/LC_MESSAGES/gcc.mo
%ifarch x86_64
%{_prefix}/lib/32
%endif
%{_prefix}/lib/gcc
%if %{have_lib64}
%{_prefix}/lib64/libstdc++.so
%{_prefix}/lib64/libstdc++.a
%{_prefix}/lib64/libstdc++.la
%{_prefix}/lib64/libsupc++.a
%{_prefix}/lib64/libsupc++.la
%{_prefix}/lib64/libgcc_s.so
%else
%{_prefix}/lib/libstdc++.so
%{_prefix}/lib/libstdc++.a
%{_prefix}/lib/libstdc++.la
%{_prefix}/lib/libsupc++.a
%{_prefix}/lib/libsupc++.la
%{_prefix}/lib/libgcc_s.so
%endif

%files runtime
%defattr(-,root,root)
%if %{have_lib64}
%{_prefix}/lib64/libstdc++.so.*
%{_prefix}/lib64/libgcc_s.so.*
%else
%{_prefix}/lib/libstdc++.so.*
%{_prefix}/lib/libgcc_s.so.*
%endif
