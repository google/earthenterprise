%define _prefix /opt/google/share/searchexample
%define _builddir .

Summary: Google Earth Enterpise Search Framework Example Database.
Name: GoogleEarthSearchExample
Version: 3.1.1
Release: %{rel_date}
License: Google Corporation
Group: Applications/GIS
Vendor: Google
URL: http://www.google.com
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildArch: noarch

%description
The Search Example Database is a part of the Google Earth Enterpise
Search Framework.

#%prep
#%setup -q

#%build

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

# make the basedir
%__install -d --mode=755 %{buildroot}/%{_prefix}

# copy everything except the .svn dirs
cp searchexample %{buildroot}/%{_prefix}
cp searchexample_db.gz %{buildroot}/%{_prefix}
cp ../java/src/com/google/earth/search/plugin/ExamplePlugin.java %{buildroot}/%{_prefix}

# fix perms
chmod 555 %{buildroot}/%{_prefix}/searchexample
chmod 444 %{buildroot}/%{_prefix}/searchexample_db.gz
chmod 444 %{buildroot}/%{_prefix}/ExamplePlugin.java

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root,0755)
%{_prefix}
