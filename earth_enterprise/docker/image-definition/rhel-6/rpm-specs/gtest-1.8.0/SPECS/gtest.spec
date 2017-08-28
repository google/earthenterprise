%define _topdir /root/opengee/rpm-specs/gtest-1.8.0
%define name    gtest-devtoolset2
%define release 1
%define version 1.8.0
%define buildroot %{_topdir}/%name-%{version}-root

BuildRoot: %{buildroot}
Summary: Shared library and development headers for gtest
License: BSD
Name: %{name}
Version: %{version}
Release: %{release}
Source: release-%{version}.tar.gz
Prefix: /opt/rh/devtoolset-2/root
Group: Development/Tools
BuildRequires: devtoolset-2-toolchain

%description
A unit test framework for C++.

You need to source /opt/rh/devtoolset-2/enable to add the shared library to
`ldconfig`'s path.

%prep
%setup -q -n googletest-release-%{version}

%build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
cd ..

%install
cd build
make install DESTDIR="$RPM_BUILD_ROOT"/opt/rh/devtoolset-2/root
cd ..

%files
%defattr(-,root,root)
/opt/rh/devtoolset-2/root/usr/lib
/opt/rh/devtoolset-2/root/usr/include
