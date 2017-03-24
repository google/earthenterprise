%define _prefix /opt/google
%define _defaultdocdir %_prefix/doc/packages
%define _mandir %_prefix/share/man
%define buildpath %_prefix/bin:/bin:/usr/bin
%define debug_package %{nil}

%define tarversion 3.0.1 

# threads
# values: pthreads, none
%define threads pthreads

Summary:	Xerces-C++ validating XML parser
Name:		xerces-c-ge
Version:	3.0.1
Release:	0
URL:		http://xml.apache.org/xerces-c/
Source0:	xerces-c-%{tarversion}.tar.gz
License:	Apache
Group:		Libraries
BuildRoot:	%{_tmppath}/%{name}-root
Prefix:		%_prefix
AutoReqProv: no
BuildRequires: gcc-ge >= 4.1.1
Requires:      gcc-ge-runtime >= 4.1.1

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
Xerces-C++ is a validating XML parser written in a portable subset of C++.
Xerces-C++ makes it easy to give your application the ability to read and
write XML data. A shared library is provided for parsing, generating,
manipulating, and validating XML documents.

The parser provides high performance, modularity, and scalability. Source
code, samples and API documentation are provided with the parser. For
portability, care has been taken to make minimal use of templates, no RTTI,
and minimal use of #ifdefs.

%package devel
Requires:	xerces-c-ge = %{version}
Group:		Development/Libraries
Summary:	Header files for Xerces-C++ validating XML parser
AutoReqProv: no

%description devel
Header files you can use to develop XML applications with Xerces-C++.

Xerces-C++ is a validating XML parser written in a portable subset of C++.
Xerces-C++ makes it easy to give your application the ability to read and
write XML data. A shared library is provided for parsing, generating,
manipulating, and validating XML documents.

%prep
%setup -q -n xerces-c-%{tarversion}

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
export XERCESCROOT=$RPM_BUILD_DIR/xerces-c-%{tarversion}
cd $XERCESCROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
./configure --enable-msgloader-inmemory \
  --enable-netaccessor-socket \
  --prefix=$RPM_BUILD_ROOT%{prefix}
make 

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-root" ] ; then rm -rf %{buildroot} ; fi
export XERCESCROOT=$RPM_BUILD_DIR/xerces-c-%{tarversion}
cd $XERCESCROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath
make install
rm -f $RPM_BUILD_ROOT%{prefix}/lib/libxerces-depdom.so*
rm -rf $RPM_BUILD_ROOT%{prefix}/bin
rm -f $RPM_BUILD_ROOT%{prefix}/lib/libxerces-*a
rm -rf $RPM_BUILD_ROOT%{prefix}/lib/pkgconfig
mkdir -p $RPM_BUILD_ROOT%{prefix}/share/xerces-c
cp -a $XERCESCROOT/samples $RPM_BUILD_ROOT%{prefix}/share/xerces-c

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(755,root,root)
%{prefix}/lib/libxerces-c-*so*

%files devel
%defattr(-,root,root)
%{prefix}/include/xercesc
%{prefix}/share/xerces-c/samples
%{prefix}/lib/libxerces-c.so

