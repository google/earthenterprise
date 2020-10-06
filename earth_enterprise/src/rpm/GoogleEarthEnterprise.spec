
%define debug_package %{nil}

Name: GoogleEarthEnterprise
Summary: Google Earth Enterprise tools
Version: 3.1.1
Release: %{rel_date}

License: Google Corporation
Group: Applications/GIS
Vendor: Google
URL: http://www.google.com


%define apachever 2.2.2
%define gccver 4.1.1
%define requirements  libtiff-ge >= 3.7.4, gdal-ge >= 1.3.2, xerces-c-ge >= 2.7.0, qt-ge >= 3.3.6


BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: jdk-ge, ant-ge

%define _builddir .
%define gebindir /opt/google/bin
%define gelibdir /opt/google/lib
%define gesharedir /opt/google/share

ExclusiveArch: i686
ExclusiveArch: x86_64


%description
Google Earth Fusion is a set of tools for the manipulation and creation of GIS
databases from source imagery and vector data, which can then be served by
Google Earth Server

%package -n GoogleEarthFusionPro
Summary: Google Earth Fusion tools
Group: Applications/GIS
Obsoletes: EarthFusion
#Obsoletes: KeyholeFusionPro
#Obsoletes: KeyholeFusionLT
#Obsoletes: KeyholeFusionGoogle
Conflicts: GoogleEarthFusionLT
Conflicts: GoogleEarthFusionGoogle
AutoReqProv: no
Requires: %{requirements}, GoogleEarthCommon >= %{version}


%description -n GoogleEarthFusionPro
Google Earth Fusion is a set of tools for the manipulation and creation of GIS
databases from source imagery and vector data, which can then be served by
Google Earth Server


%package -n GoogleEarthFusionLT
Summary: Google Earth fusion tools
Group: Applications/GIS
Obsoletes: EarthFusion
#Obsoletes: KeyholeFusionPro
#Obsoletes: KeyholeFusionLT
#Obsoletes: KeyholeFusionGoogle
Conflicts: GoogleEarthFusionPro
Conflicts: GoogleEarthFusionGoogle
AutoReqProv: no
Requires: %{requirements}, GoogleEarthCommon >= %{version}


%description -n GoogleEarthFusionLT
Google Earth Fusion is a set of tools for the manipulation and creation of GIS
databases from source imagery and vector data, which can then be served by
Google Earth Server

%package -n GoogleEarthFusionGoogle
Summary: Google Earth fusion tools
Group: Applications/GIS
#Obsoletes: KeyholeFusionPro
#Obsoletes: KeyholeFusionLT
#Obsoletes: KeyholeFusionGoogle
#Obsoletes: KeyholeFusionInternal
Conflicts: GoogleEarthFusionPro
Conflicts: GoogleEarthFusionLT
AutoReqProv: no
Requires: %{requirements}, GoogleEarthCommon >= %{version}

%description -n GoogleEarthFusionGoogle
Google EarthFusion is a set of tools for the manipulation and creation of GIS
databases from source imagery and vector data, which can then be served by
GoogleEarthServer. This is the Google-internal version.

%package -n GoogleEarthServer
Summary: An extension to Apache to serve GoogleEarth databases
Group: Applications/GIS
Conflicts: EarthServer
Obsoletes: KeyholeServerColo
Obsoletes: KeyholeServerCustom
Obsoletes: KeyholeServer
Requires: apache-ge >= %{apachever}, GoogleEarthCommon >= %{version}, tomcat-ge, postgresql-ge, gcc-ge-runtime >= %{gccver}
AutoReqProv: no

%description -n GoogleEarthServer
Google Earth Server is an extension to Apache which can serve GIS databases to
clients such as Google Earth Pro and Google Earth LT.  It is configured to work with a custom installation of apache.

%package -n GoogleEarthCommon
Summary: A set of files used by both the server and fusion
Group: Applications/GIS
#Obsoletes: KeyholeCommon
AutoReqProv: no

%description -n GoogleEarthCommon
A set of files used by both the server and fusion

%package -n GoogleEarthServerDisconnected
Summary: Extra tools to support disconnected servers
Group: Applications/GIS
Requires: GoogleEarthServer = %{version}
AutoReqProv: no

%description -n GoogleEarthServerDisconnected
Extra tools to support disconnected servers


# %prep
# %build

%install
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
cat fusion.common fusion.pro               > %{_tmppath}/tmp_fusionpro.files
cat fusion.common fusion.lt                > %{_tmppath}/tmp_fusionlt.files
cat fusion.common fusion.pro fusion.google > %{_tmppath}/tmp_fusiongoogle.files
mkdir -p %{buildroot}/opt/google
mkdir -p %{buildroot}/etc/opt/google
mkdir -p %{buildroot}/var/opt/google/log
mkdir -p %{buildroot}/var/opt/google/run
mkdir -p %{buildroot}/opt/google/share/icons
if [ -x /usr/bin/getconf ] ; then
  NRPROC=$(/usr/bin/getconf _NPROCESSORS_ONLN)
   if [ $NRPROC -eq 0 ] ; then
    NRPROC=1
  fi
else
  NRPROC=1
fi
NRPROC=$(($NRPROC*2))
rpmbuilddir=`pwd`

# build and install the java portion of our tree
# make sure we do a clean build
/opt/google/ant/bin/geant -f ../../java/ant/build.xml clean
/opt/google/ant/bin/geant -DINSTALLDIR=%{buildroot} -f ../../java/ant/build.xml install

# temporarily remove files that we don't want to ship yet
# for example
rm %{buildroot}/opt/google/search/plugins/AddressRoutingPlugin.jar
rm %{buildroot}/opt/google/search/plugins/AddressRoutingPlugin.properties
rm %{buildroot}/opt/google/search/plugins/ReverseGeocodingPlugin.jar
rm %{buildroot}/opt/google/search/plugins/ReverseGeocodingPlugin.properties
rm %{buildroot}/opt/google/search/plugins/StreetGeocodingPlugin.jar
rm %{buildroot}/opt/google/search/plugins/StreetGeocodingPlugin.properties
rm %{buildroot}/opt/google/search/tabs/Directions.gestd
rm %{buildroot}/opt/google/search/tabs/Reverse_Geocoder.gestd
rm %{buildroot}/opt/google/search/tabs/Street_Geocoder.gestd


# build and install the scons portion of our tree
cd .. ; python2.7 /usr/bin/scons -j $NRPROC release=1 installdir=%{buildroot} install

# make some convenience symlinks
ln -s /etc/opt/google     %{buildroot}/opt/google/etc
ln -s /var/opt/google/run %{buildroot}/opt/google/run
ln -s /var/opt/google/log %{buildroot}/opt/google/log

# install the docs
mkdir -p %{buildroot}/%{gesharedir}/doc
mkdir -p %{buildroot}/%{gesharedir}/doc/manual
( cd ../docs/landing_page ; tar c -f - --exclude=\*/.svn --exclude=\*~ * ) | \
( cd %{buildroot}/%{gesharedir}/doc/manual ; tar x -f - )

# install the default dbs
( cd .. ; tar c -f - --exclude=\*/.svn --exclude=\*~ datasets ) | \
( cd %{buildroot}/%{gesharedir} ; tar x -f - )


# do some cleanup
find %{buildroot} -name .sconsign -print0 | xargs -r --null rm ;
find %{buildroot} -name .svn -print0 | xargs -r --null rm ;

# validate that we have all of our shared libs that we need to run
echo "Verifying compatibility of system libraries ... ";
$rpmbuilddir/../installer/lib/ValidateLibs.pl --bindir=%{buildroot}/opt/google/bin --altlibdir=%{buildroot}/opt/google/lib



%clean
if [ "%{buildroot}" == "%{_tmppath}/%{name}-%{version}-buildroot" ] ; then rm -rf %{buildroot} ; fi
rm -f %{_tmppath}/tmp_fusionpro.files
rm -f %{_tmppath}/tmp_fusionlt.files
rm -f %{_tmppath}/tmp_fusiongoogle.files


%files -n GoogleEarthFusionPro -f %{_tmppath}/tmp_fusionpro.files

%files -n GoogleEarthFusionGoogle -f %{_tmppath}/tmp_fusiongoogle.files

%files -n GoogleEarthFusionLT -f %{_tmppath}/tmp_fusionlt.files

%files -n GoogleEarthServer -f server.files

%files -n GoogleEarthCommon -f common.files

%files -n GoogleEarthServerDisconnected -f server.disconnected
