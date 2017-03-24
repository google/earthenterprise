%define         _prefix         /opt/google
%define         qtdir		%{_prefix}/qt
%define debug_package %{nil}

Summary:	The Qt GUI application framework
Name:		qt-ge
Version:	3.3.6
Release:	8
License:	GPL / QPL
Group:		X11/Libraries
Source0:	ftp://ftp.trolltech.com/qt/source/qt-x11-commercial-%{version}.tar.bz2

# turns off usage of -rpath so we can use LD_RUN_PATH
Patch0:         qt-rpath.patch
%define         qtrunpath	%qtdir/lib:%qtdir/plugins/designer

Patch1:		qt-ge-noGLU.patch


BuildRequires:	flex
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
AutoReqProv:	no

BuildRequires:  gcc-ge >= 4.1.1
Requires:	gcc-ge-runtime >= 4.1.1

BuildRequires:  /usr/include/GL/gl.h
BuildRequires: libpng-devel
Requires: libpng
BuildRequires: libmng-devel
Requires: libmng

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define ldrunpath /opt/google/lib:%qtrunpath
BuildRequires: /usr/lib/libGL.so
Requires: /usr/lib/libGL.so.1
%endif

%ifarch x86_64
%define ldrunpath /opt/google/lib64:/opt/google/lib:%qtrunpath
BuildRequires: /usr/lib64/libGL.so
Requires: /usr/lib64/libGL.so.1
%endif

%description
Qt is a GUI software toolkit which simplifies the task of writing and
maintaining GUI (Graphical User Interface) applications for the X
Window System. Qt is written in C++ and is fully object-oriented.

This package contains the shared library needed to run Qt
applications, as well as the README files for Qt.

%package devel
Summary:	Development files and documentation for the Qt GUI toolkit
Group:		X11/Development/Libraries
Requires:	%{name} = %{version}-%{release}
AutoReqProv:	no

%description devel
Contains the files necessary to develop applications using Qt: header
files, the Qt meta object compiler, man pages, HTML documentation and
example programs. See http://www.trolltech.com/ for more information
about Qt, or file:/usr/share/doc/%{name}-%{version}/html/index.html
for Qt documentation in HTML.

%prep
%setup -q -n qt-x11-commercial-%{version}
%patch0 -p1
%patch1 -p1


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
export QTDIR=`/bin/pwd`
export PATH=$QTDIR/bin:/opt/google/bin:/bin:/usr/bin
export LD_LIBRARY_PATH=$QTDIR/lib
export LD_RUN_PATH=%ldrunpath
export CustomerID="38542"
export LicenseID="1517223"
export Licensee="Mark Wheeler"
export Products="qt-enterprise"
export ExpiryDate=2009-02-05
export LicenseKey=9EKD-3NGX-3XGA
if [ `uname -m` = x86_64 ] ; then
  export PLATFORM='-platform linux-g++-64'
fi
export QMAKE_LIBS_OPENGL='-lGL -lXmu'
./configure $PLATFORM \
    -prefix %qtdir \
    -no-exceptions \
    -system-libpng \
    -system-libjpeg \
    -qt-imgfmt-jpeg \
    -system-libmng \
    -qt-imgfmt-mng \
    -system-zlib \
    -thread <<EOF
yes
EOF
make -j $NRPROC src-qmake src-moc sub-src sub-tools

%install
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT
if [ -f /etc/sysconfig/grhat ] ; then
	mkdir -p $RPM_BUILD_ROOT/opt
	mkdir -p $RPM_BUILD_ROOT/usr/local
	ln -s ../../opt $RPM_BUILD_ROOT/usr/local/opt
fi
make INSTALL_ROOT=$RPM_BUILD_ROOT install
if [ -f /etc/sysconfig/grhat ] ; then
	rm -rf $RPM_BUILD_ROOT/usr
fi

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%post	
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(755,root,root,755)
%dir %{qtdir}
%{qtdir}/bin/qtconfig
%defattr(644,root,root,755)
%dir %{qtdir}/lib
%{qtdir}/lib/lib*.so.*
%dir %{qtdir}/plugins
%{qtdir}/translations


%files devel
%defattr(755,root,root,755)
%dir %{qtdir}
%{qtdir}/bin/assistant
%{qtdir}/bin/designer
%{qtdir}/bin/linguist
%{qtdir}/bin/lrelease
%{qtdir}/bin/lupdate
%{qtdir}/bin/moc
%{qtdir}/bin/qm2ts
%{qtdir}/bin/qmake
%{qtdir}/bin/uic
%defattr(644,root,root,755)
%dir %{qtdir}/doc
%{qtdir}/doc/html
%{qtdir}/include
%dir %{qtdir}/lib
%{qtdir}/lib/libdesignercore.*
%{qtdir}/lib/libeditor.*
%{qtdir}/lib/libqassistantclient.*
%{qtdir}/lib/libqt-mt.prl
%{qtdir}/lib/libqt-mt.so
%{qtdir}/lib/libqt-mt.la
%{qtdir}/lib/libqui.prl
%{qtdir}/lib/libqui.so
%{qtdir}/lib/pkgconfig
%{qtdir}/mkspecs
%{qtdir}/phrasebooks
%dir %{qtdir}/plugins
%{qtdir}/plugins/designer
%{qtdir}/templates
