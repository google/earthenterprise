%define _prefix /opt/google
%define _mandir %_prefix/share/man
%define _infodir %_prefix/share/info
%define _libdir %_prefix/lib
%define debug_package %{nil}

Summary: GNU Debugger - Packaged by Google for a consistent build environment
Name: gdb-ge
Version: 6.5
Release: 3
Source0: gdb-%{version}.tar.bz2
License: GPL
Group: Development/Debugging
URL: http://www.gnu.org/software/gdb
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1
BuildRequires: ncurses-devel
Requires: ncurses

ExclusiveArch: i686
ExclusiveArch: x86_64


%ifarch i686
%endif

%ifarch x86_64
%endif

%description
C and C++ debugger.
This version has been compiled by Google.


%prep
%setup -q -c

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
export PATH='/opt/google/bin:/bin:/usr/bin'
export LD_RUN_PATH='/opt/google/lib'
CFLAGS='-O2' \
../gdb-%{version}/configure \
    --prefix=%{_prefix} \
    --mandir=%_mandir \
    --infodir=%_infodir

make -j $NRPROC

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
cd build
export PATH='/opt/google/bin:/bin:/usr/bin'
export LD_RUN_PATH='/opt/google/lib'
make install DESTDIR=$RPM_BUILD_ROOT
rm -rf $RPM_BUILD_ROOT%{_libdir}
rm -rf $RPM_BUILD_ROOT%{_prefix}/include
rm -rf $RPM_BUILD_ROOT%{_prefix}/share/locale
rm  $RPM_BUILD_ROOT%{_infodir}/bfd.*
rm  $RPM_BUILD_ROOT%{_infodir}/configure.*
rm  $RPM_BUILD_ROOT%{_infodir}/standards.*

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_prefix}
