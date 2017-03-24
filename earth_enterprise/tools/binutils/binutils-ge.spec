%define _prefix /opt/google
%define _mandir %_prefix/share/man
%define _infodir %_prefix/share/info
%define _libdir %_prefix/lib
%define debug_package %{nil}

Summary: GNU Binutils - Packaged by Google for a consistent build environment
Name: binutils-ge
Version: 2.16.91.0.7
Release: 4
Source0: binutils-2.16.91.0.7.tar.bz2
Patch0: x86-64-biarch.patch
License: GPL, LGPL
Group: Development/Languages/Other
URL: http://www.gnu.org/software/binutils/binutils.html
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
AutoReqProv: no
BuildRequires: bison, flex

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -O2
%endif

%ifarch x86_64
%define optflags -O2
%endif

%description
C compiler utilities: ar, as, gprof, ld, nm, objcopy, objdump, ranlib,
size, strings, and strip.

These utilities are needed whenever you want to compile a program or
kernel.

This version has been compiled by Google.


%prep
%setup -q -n binutils-%{version}
%patch0


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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' \
./configure \
    --prefix=%{_prefix}  \
    --mandir=%_mandir \
    --infodir=%_infodir \
    --enable-shared \
    --with-gnu-ld \
    --with-gnu-as \
    --disable-nls

# makeinfo's being a pain on older platforms
export EXTRA_MAKEARGS='MAKEINFO=true'
make configure-bfd
make headers -C bfd
make $EXTRA_MAKEARGS -j $NRPROC

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
export EXTRA_MAKEARGS='MAKEINFO=true'
make $EXTRA_MAKEARGS install DESTDIR=$RPM_BUILD_ROOT

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_prefix}
