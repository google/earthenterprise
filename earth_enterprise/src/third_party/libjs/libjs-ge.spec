%define PACKAGE_NAME libjs-ge
%define _prefix /opt/google
%define _libdir %{_prefix}/lib
%define _defaultdocdir %_prefix/share/doc/packages
%define _mandir %_prefix/share/man
%define firefoxver 2.0.0.20
%define jsver 1.7.0
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

Summary:      SpiderMonkey Javascript Engine
Name:         %PACKAGE_NAME
Version:      %firefoxver
Release:      0
License:      MPL (Mozilla Public License)
Group:        Development/Libraries/C and C++
Source:       firefox-%{version}-source.tar.bz2
Source1:      js-%{jsver}.tar.gz
Source2:      gejsapi.h
Source3:      gejsapi.c

BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://www.mozilla.org/js/spidermonkey
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1

ExclusiveArch: i686
ExclusiveArch: x86_64

%define extraconf %{nil}
%ifarch i686
%define ldrunpath %_prefix/lib
%endif

%ifarch x86_64
%define ldrunpath %_prefix/lib64:%_prefix/lib
%define extraconf --enable-64bit
%endif

%description
Mozilla's C implementation of JavaScript, packaged for distribution with
Google Earth Fusion

%package devel
Summary:      SpiderMonkey Javascript Engine - Development files
Group:        Development/Libraries/C and C++
License:      MPL (Mozilla Public License)
Requires: %{name} >= %{version}
AutoReqProv: no

%description devel
SpiderMonkey development files


%prep
%setup -q -n %{name}-%{version} -c -T
tar xjf $RPM_SOURCE_DIR/firefox-%{version}-source.tar.bz2 mozilla/js/src mozilla/nsprpub
cd mozilla
tar xzf $RPM_SOURCE_DIR/js-%{jsver}.tar.gz js/src/config js/src/editline
cd ..
chmod -Rf a+rX,g-w,o-w .
cp $RPM_SOURCE_DIR/gejsapi.h mozilla/js/src
cp $RPM_SOURCE_DIR/gejsapi.c mozilla/js/src


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
mkdir build
cd build
../mozilla/nsprpub/configure \
    --enable-optimize \
    --disable-debug \
    %extraconf
make -j $NRPROC
cd ../mozilla/js/src
INCLUDES='-I../../../build/dist/include/nspr' \
OTHER_LIBS='-L../../../build/dist/lib' \
JS_THREADSAFE=1 BUILD_OPT=1 \
XCFLAGS=-fPIC \
make LIB_CFILES='$(JS_CFILES) gejsapi.c' -f Makefile.ref


%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
INCDIR=$RPM_BUILD_ROOT%{_prefix}/include
LIBDIR=$RPM_BUILD_ROOT%{_libdir}
#DOCDIR=$RPM_BUILD_ROOT/%_defaultdocdir/%name
install -d -m 755 $INCDIR/libjs
install -d -m 755 $LIBDIR
#install -d -m 755 $DOCDIR
install -m 755 mozilla/js/src/Linux_All_OPT.OBJ/libjs.so $LIBDIR
install -m 755 build/dist/lib/libnspr4.so $LIBDIR
cat >$INCDIR/gejsapi.h <<EOF
#ifndef _GE_NSAPI

#ifndef XP_UNIX
#define XP_UNIX
#endif
#ifndef SVR4
#define SVR4
#endif
#ifndef SYSV
#define SYSV
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef POSIX_SOURCE
#define POSIX_SOURCE
#endif
#ifndef HAVE_LOCALTIME_R
#define HAVE_LOCALTIME_R
#endif
#ifndef JS_THREADSAFE
#define JS_THREADSAFE
#endif

#include <libjs/gejsapi.h>

#endif /* _GE_NSAPI */
EOF
install -m 644 mozilla/js/src/gejsapi.h  $INCDIR/libjs
install -m 644 mozilla/js/src/jsapi.h    $INCDIR/libjs
install -m 644 mozilla/js/src/jspubtd.h  $INCDIR/libjs
install -m 644 mozilla/js/src/jstypes.h  $INCDIR/libjs
install -m 644 mozilla/js/src/jscompat.h $INCDIR/libjs
install -m 644 mozilla/js/src/jsotypes.h $INCDIR/libjs
install -m 644 mozilla/js/src/jslong.h   $INCDIR/libjs
install -m 644 mozilla/js/src/jsosdep.h  $INCDIR/libjs
install -m 644 mozilla/js/src/Linux_All_OPT.OBJ/jsautocfg.h $INCDIR/libjs
install -m 644 mozilla/js/src/jsproto.tbl $INCDIR/libjs
install -m 644 mozilla/js/src/jsconfig.h $INCDIR/libjs

perl -n -e "print; last if /END LICENSE BLOCK/;" <mozilla/js/src/jsapi.h >LICENSE
chmod 644 LICENSE


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc LICENSE
%{_libdir}/*.so

%files devel
%defattr(-, root, root)
%{_prefix}/include/libjs
%{_prefix}/include/gejsapi.h
