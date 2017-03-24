%define PACKAGE_NAME expat-ge
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}


Summary:      XML Parser Toolkit
Name:         %PACKAGE_NAME
Version:      2.0.1
Release:      0
License:      MPL
Group:        Development/Libraries/C and C++
Source:       expat-%{version}.tar.gz
#Patch0:        expat-DESTDIR.patch
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://expat.sourceforge.net/
AutoReqProv: no
 
BuildRequires: zlib-devel
Requires: zlib
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
Expat is an XML 1.0 parser written in C.

%package devel
Summary:      XML Parser Toolkit - Development files
Group:        Development/Libraries/C and C++
License:      MPL
Requires: %{name} = %{version}
Requires: zlib-devel
AutoReqProv: no

%description devel
expat development files



%prep
%setup -n expat-%{version}
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
CFLAGS='%{optflags}' CXXFLAGS='%{optflags}' ./configure \
    --mandir=%{_mandir} \
    --prefix=%{_prefix}
make -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make DESTDIR=$RPM_BUILD_ROOT install
rm doc/xmlwf.1
rm -rf $RPM_BUILD_ROOT%{_mandir}
rm -rf $RPM_BUILD_ROOT%{_prefix}/bin
rm -rf $RPM_BUILD_ROOT%{_prefix}/lib/*.a
ln -s `readlink $RPM_BUILD_ROOT%{_prefix}/lib/libexpat.so.1` $RPM_BUILD_ROOT%{_prefix}/lib/libexpat.so.0

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc COPYING Changes README 
%{_prefix}/lib/*.so.*

%files devel
%defattr(-, root, root)
%{_prefix}/include/*
%{_prefix}/lib/*.so
%{_prefix}/lib/*.la
