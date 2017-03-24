%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define _libdir %_prefix/lib
%define fontdir %_prefix/share/fonts
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Name:         libsgl-ge
License:      Google
Group:        System/Libraries
Autoreqprov:  no
Summary:      A library for vector rasterization
Version:      0.8
Release:      6
Source:       sgl.%{version}.tar.gz
Source1:      luxisr.ttf
#Source2:      SkUserConfig.h
Patch0:       libsgl-64bit.patch
Patch1:       libsgl-removeprintf.patch
Patch2:       libsgl-userconfig.patch
Patch3:       libsgl-fonthost.patch
Patch4:       libsgl-headerinstall.patch
Patch5:       libsgl-sktdarray.patch
Patch6:       libsgl-png.patch
BuildRoot:    %{_tmppath}/%{name}-%{version}-build

%description
SKIA Vector rasterization library

BuildRequires: gcc-ge >= 4.1.1
BuildRequires: /usr/include/freetype2

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
Summary:      libsgl development files
Group:        Development/Libraries/C and C++
Autoreqprov:  no
Requires:     /usr/include/freetype2

%description devel
libsgl development files

%prep
%setup -q -c
chmod -R u+w .
mv skia/* .
rmdir skia
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1


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
CFLAGS='%{optflags} -DSK_FONTFACE=\"Sans\"' \
CXXFLAGS='%{optflags} -DSK_FONTFACE=\"Sans\"' \
./configure \
    --prefix=%{_prefix} \
    --libdir=%{_libdir} \
    --enable-float \
    --enable-effects \
    --enable-fontpath \
    --with-fontpath=%{fontdir}/sans.ttf \
    --with-freetype \
    --with-png
make bin_PROGRAMS='' -j $NRPROC

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make bin_PROGRAMS='' DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT%{_prefix}/include/sgl
chmod 755 $RPM_BUILD_ROOT%{_prefix}/include/sgl
mv $RPM_BUILD_ROOT%{_prefix}/include/*.h $RPM_BUILD_ROOT%{_prefix}/include/sgl
cp include/SkUserConfig.h $RPM_BUILD_ROOT%{_prefix}/include/sgl
mkdir -p $RPM_BUILD_ROOT%{fontdir}
chmod 755 $RPM_BUILD_ROOT%{fontdir}
cp $RPM_SOURCE_DIR/luxisr.ttf  $RPM_BUILD_ROOT%{fontdir}
ln -s luxisr.ttf $RPM_BUILD_ROOT%{fontdir}/sans.ttf

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
%files
%defattr(-, root, root)
%{fontdir}

%files devel
%defattr(-, root, root)
%{_libdir}/lib*.a
%{_prefix}/include/sgl
