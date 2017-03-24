%define _prefix /opt/google/share/geplaces
%define _builddir .

Summary: Google Earth Enterpise Search Framework Places Database.
Name: GoogleEarthPlacesDB
Version: 3.1.1
Release: %{rel_date}
License: Google Corporation
Group: Applications/GIS
Vendor: Google
URL: http://www.google.com
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildArch: noarch

%description
The Places Database is a part of the Google Earth Enterpise Search Framework.

#%prep
#%setup -q

#%build

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

# make the basedir
%__install -d --mode=755 %{buildroot}/%{_prefix}

# copy everything except the .svn dirs
cp geplaces %{buildroot}/%{_prefix}
cp geplaces_db.gz %{buildroot}/%{_prefix}

# fix perms
chmod 555 %{buildroot}/%{_prefix}/geplaces
chmod 444 %{buildroot}/%{_prefix}/geplaces_db.gz

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root,0755)
%{_prefix}
