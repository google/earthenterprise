%define _prefix /opt/google/share/tutorials/fusion
%define _builddir .

Summary: A set of tutorial files for Google Earth Fusion
Name: GoogleEarthFusionTutorial
Version: 3.1.1
Release: %{rel_date}
License: Google Corporation - All Rights Reserved
Group: Applications/GIS
Vendor: Google
URL: http://www.google.com
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildArch: noarch

%description
GoogleEarthFusionTutorial is a set of imagery, terrain and vector sources
to build a sample database and help the user become familiar
with usage of the product

#%prep
#%setup -q

#%build

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

# make the basedir
%__install -d --mode=755 %{buildroot}/%{_prefix}

# copy everything except the .svn dirs
( cd FusionTutorial ; tar c -f - --exclude=\*/.svn --exclude=\*~ * ) | \
( cd %{buildroot}/%{_prefix} ; tar x -f - )

# fix perms
find %{buildroot}/%{_prefix} -type d -print0 | \
xargs -r --null chmod 0755
find %{buildroot}/%{_prefix} -type f -print0 | \
xargs -r --null chmod 0444

%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi

%files
%defattr(-,root,root)
%{_prefix}/Imagery
%{_prefix}/Terrain
%{_prefix}/Vector
%{_prefix}/KML
