%define apache_ver   2.4.46
%define mm_ver       1.4.2
%define debug_package %{nil}

%define REQ_EXPAT_VER 2.0.0

# Apache user
%define apacheuser   IA_GEAPACHE_USER
%define apachegroup  IA_GEGROUP

# Name of apache-ge binary
%define target      gehttpd

# Where to live
%define _prefix /opt/google
%define _defaultdocdir %_prefix/share/doc/packages
%define httpd_root  %{_prefix}/%{target}
%define confdir     %{httpd_root}/conf
%define docdir      %{httpd_root}/htdocs
%define bindir      %{httpd_root}/bin
%define libexecdir  %{httpd_root}/modules
%define buildpath %_prefix/bin:/bin:/usr/bin

Summary: The most widely used Web server on the Internet.
Name: apache-ge
Version: %{apache_ver}
Release: 0
Group: System Environment/Daemons
Source: httpd-%{version}.tar.gz
Source1: mm-%{mm_ver}.tar.gz
Source2: ge-server.png
Source4: apache-ge.logrotate
Source5: SSL-Certificate-Creation
Source6: apache-ge-index.shtml
Source7: apache-ge-index.conf
# Source8: %{mod_jk_name}-%{mod_jk_ver}-src.tar.gz
# Patch to enable httpd manual access by default.
Patch1:   apache-ge-enablemanual.patch


License: Apache Software License
BuildRoot: %{_tmppath}/apache-ge-%{version}-root
BuildPrereq: findutils
BuildPrereq: perl, gdbm-devel
Requires: /etc/mime.types, gawk, file, /usr/bin/find
Prereq: /sbin/chkconfig, /bin/mktemp, /bin/rm, /bin/mv, /bin/sed, /etc/mailcap, grep
Prereq: sh-utils, textutils, /usr/sbin/useradd
Requires: expat-ge >= %REQ_EXPAT_VER
BuildRequires: expat-ge-devel >= %REQ_EXPAT_VER
AutoReqProv: no

BuildRequires: gcc-ge >= 4.1.1
Requires: gcc-ge-runtime >= 4.1.1

BuildRequires: openldap-ge-devel >= 2.3.39-0
Requires: openldap-ge >= 2.3.39-0

BuildRequires: libtool
BuildConflicts: libtool-32bit

BuildRequires: openssl-ge-devel >= 0.9.8
BuildConflicts: openssl-devel

ExclusiveArch: i686
ExclusiveArch: x86_64

%ifarch i686
%define optflags -march=i686 -O2
%define ldrunpath %_prefix/lib
%define libstdcpppath /opt/google/lib/libstdc++.so.6
%endif

%ifarch x86_64
%define optflags -O2
%define ldrunpath %_prefix/lib64:%_prefix/lib
%define libstdcpppath /opt/google/lib64/libstdc++.so.6
%endif


%description
Apache is a powerful, full-featured, efficient, and freely-available
Web server. Apache is also the most popular Web server on the
Internet.

%package devel
Group: Development/Libraries
Summary: Development tools for the Apache Web server.
AutoReqProv: no

%description devel
The apache-devel package contains the APXS binary and other files that
you need to build Dynamic Shared Objects (DSOs) for Apache.

If you are installing the Apache Web server and you want to be able to
compile or develop additional modules for Apache, you need to install
this package.

%prep
%setup -q -c
%setup -q -c -T -D -b 1
# %setup -q -c -T -D -b 8
cd $RPM_BUILD_DIR/apache-ge-%{version}
cd httpd-%{version}
%patch1 -p1
cd ..

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

CFLAGS="%{optflags} -fPIC"; export CFLAGS
LIBS="-lpthread" ; export LIBS

# configure/make the mm library
cd mm-%{mm_ver}
./configure --disable-shared
make -j $NRPROC
cd ..

# set the default port to 80
export PORT=80

# grhat specific stuff
if [ -f /etc/sysconfig/grhat ] ; then
    export INCLUDES='-I%{_prefix}/include -I/usr/kerberos/include'
else
    export INCLUDES='-I%{_prefix}/include'
fi
export LDFLAGS='-L%{_prefix}/lib -Wl,-rpath,%{_prefix}/lib'

EAPI_MM=../mm-%{mm_ver}

cd httpd-%{version}
./configure \
   --with-program-name=%{target} \
    --with-port=$PORT \
    --prefix=%{httpd_root} \
    --enable-mods-shared="all" \
    --with-ssl=%{_prefix} \
    --enable-ssl=shared \
    --with-expat=%{_prefix} \
    --with-mpm=worker \
    --enable-proxy \
    --enable-proxy-ajp \
    --disable-proxy-connect \
    --disable-proxy-ftp \
    --disable-proxy-http \
    --disable-proxy-balancer \
    --without-sqlite2 \
    --without-sqlite3 \
    --without-mysql \
    --with-ldap-dir=%{_prefix} \
    --with-ldap \
    --enable-ldap \
    --enable-authnz_ldap

exit

make -j $NRPROC
cd ..

# we need to build the mod_jk after we
# build apache coz mod_jk requires apxs
# chmod 755 $RPM_BUILD_ROOT%{httpd_root}/bin/apxs
# cd %{mod_jk_name}-%{mod_jk_ver}-src/jk/native
#./configure --with-apxs=$RPM_BUILD_ROOT%{httpd_root}/bin/apxs
# make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
export PATH=%buildpath
export LD_RUN_PATH=%ldrunpath


# install apache portion
cd httpd-%{version}
make \
    DESTDIR=$RPM_BUILD_ROOT \
    install

# install log rotation stuff
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
install -m644 $RPM_SOURCE_DIR/apache-ge.logrotate \
	$RPM_BUILD_ROOT/etc/logrotate.d/%{target}

# install the default web page
install -m444 $RPM_SOURCE_DIR/ge-server.png \
    $RPM_BUILD_ROOT%{docdir}/ge-server.png
install -m444 $RPM_SOURCE_DIR/apache-ge-index.shtml \
    $RPM_BUILD_ROOT%{docdir}/index.shtml


# modify perl scripts to call %{__perl}
find $RPM_BUILD_ROOT -type f | \
	xargs grep -l "/usr/local/bin/perl5" | \
	xargs perl -pi -e "s|/usr/local/bin/perl5|/usr/bin/perl|g;"
find $RPM_BUILD_ROOT -type f | \
	xargs grep -l "/usr/local/bin/perl" | \
	xargs perl -pi -e "s|/usr/local/bin/perl|/usr/bin/perl|g;"

# remove execute bits from CGIs
chmod -x $RPM_BUILD_ROOT%{httpd_root}/cgi-bin/*

# fix up apxs so that it doesn't think it's in the build root
sed "s^$RPM_BUILD_ROOT^^g" $RPM_BUILD_ROOT%{bindir}/apxs > apxs.tmp && \
cat apxs.tmp > $RPM_BUILD_ROOT%{bindir}/apxs

# create a conf.d directory next to the regular one
install -m777 -d $RPM_BUILD_ROOT%{httpd_root}/conf.d

# install index_rewrite files into conf.d
perl -p \
    -e "s,\@\@ServerRoot\@\@,%{httpd_root}," \
    $RPM_SOURCE_DIR/apache-ge-index.conf \
    > $RPM_BUILD_ROOT%{httpd_root}/conf.d/index_rewrite
chmod 444 $RPM_BUILD_ROOT%{httpd_root}/conf.d/index_rewrite


# Add a directive to include all the Google Earth server
# specific files from the conf.d directory
echo "" >> $RPM_BUILD_ROOT%confdir/%{target}.conf
echo "# Load libstdc++ so modules can use C++" >> $RPM_BUILD_ROOT%confdir/%{target}.conf
echo "LoadFile %{libstdcpppath}" >> $RPM_BUILD_ROOT%confdir/%{target}.conf
echo "" >> $RPM_BUILD_ROOT%confdir/%{target}.conf
echo "# Include Google Earth Server-specific files" >> $RPM_BUILD_ROOT%confdir/%{target}.conf
echo "Include conf.d/*.conf" >> $RPM_BUILD_ROOT%confdir/%{target}.conf

# Set the user/group directives in the conf file
# apache will run as the following user/group
perl -pi -e "s/^User \w+/User %{apacheuser}/; s/^Group \w+/Group %{apachegroup}/;" $RPM_BUILD_ROOT%confdir/%{target}.conf


# clean out stuff we don't want to package
rm -rf $RPM_BUILD_ROOT%{httpd_root}/man
rm -rf $RPM_BUILD_ROOT%{httpd_root}/error     # used to output custom
                                              # error message in different languages
rm -rf $RPM_BUILD_ROOT%{bindir}/htcacheclean
rm -rf $RPM_BUILD_ROOT%{bindir}/htdbm
rm -rf $RPM_BUILD_ROOT%{bindir}/httxt2dbm
rm -rf $RPM_BUILD_ROOT%{bindir}/logresolve
rm -rf $RPM_BUILD_ROOT%{docdir}/apache_pb.png
rm -rf $RPM_BUILD_ROOT%{docdir}/apache_pb22.gif
rm -rf $RPM_BUILD_ROOT%{docdir}/apache_pb22.png
rm -rf $RPM_BUILD_ROOT%{docdir}/apache_pb22_ani.gif
rm -rf $RPM_BUILD_ROOT%{docdir}/apache_pb.gif
rm -rf $RPM_BUILD_ROOT%{docdir}/index.html


# Get apache to run as with a user mask of 775 because getomcatuser:gegroup
# will need to write to dirs created by mod_dav (which runs as geapacheuser)
# TODO (amin): addming umask to envvars does not work. Till we find out why I'm
# adding it to the gehttpd_init script.
# echo "umask 002" >> $RPM_BUILD_ROOT%{bindir}/envvars

%clean
# rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc $RPM_BUILD_DIR/apache-ge-%{version}/httpd-%{version}/ABOUT_APACHE
%doc $RPM_BUILD_DIR/apache-ge-%{version}/httpd-%{version}/README
%doc $RPM_BUILD_DIR/apache-ge-%{version}/httpd-%{version}/CHANGES
%doc $RPM_BUILD_DIR/apache-ge-%{version}/httpd-%{version}/LICENSE
%doc $RPM_BUILD_DIR/apache-ge-%{version}/httpd-%{version}/NOTICE
%dir %{httpd_root}
%dir %{confdir}
%config %{confdir}/extra/*.conf
%config %{confdir}/original/*.conf
%config %{confdir}/original/extra/*.conf
%config(noreplace) %{confdir}/%{target}.conf
%config %{confdir}/magic

%{httpd_root}/lib/*

%dir %{httpd_root}/conf.d
%config %{httpd_root}/conf.d/index_rewrite
%config %{confdir}/mime.types

%{httpd_root}/logs
# %{_sysconfdir}/httpd/modules
%config /etc/logrotate.d/%{target}

%dir %{docdir}
%config(noreplace) %{docdir}/ge-server.png
%config(noreplace) %{docdir}/index.shtml
%{httpd_root}/manual
%{httpd_root}/cgi-bin
%{httpd_root}/icons

%dir %{libexecdir}
%{libexecdir}/httpd.exp
%{libexecdir}/*.so

%{bindir}/ab
%{bindir}/apachectl
%{bindir}/%{target}
%{bindir}/checkgid
%{bindir}/dbmmanage
%{bindir}/htdigest
%{bindir}/htpasswd
%{bindir}/envvars
%{bindir}/envvars-std
%{bindir}/apr-1-config
%{bindir}/apu-1-config

# %attr(0755,%{apacheuser},root) %dir %{_localstatedir}/cache/httpd
# %dir %{_localstatedir}/log/httpd

%files devel
%defattr(-,root,root)
%{httpd_root}/include
%{bindir}/apxs
%{httpd_root}/build/*
