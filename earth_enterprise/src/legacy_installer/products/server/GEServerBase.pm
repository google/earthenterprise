#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#-*-perl-*-

package GEServerBase;

use strict;
use Product;
use Users;
use RPMHelps;
use InstallUtils;
use TermHelps;
use prompt;

our( @ISA );
@ISA = ("Product");

our $ServerRPM  = "GoogleEarthServer";
our $ApachePidFile   = '/opt/google/gehttpd/logs/httpd.pid';
our $TomcatPidFile   = '/var/opt/google/run/getomcat.pid';
our $PostgresPidFile = '/var/opt/google/pgsql/data/postmaster.pid';
our $TomcatEnvFile   = '/opt/google/getomcat/bin/setenv.sh';
our $ApacheEnvFile   = '/opt/google/gehttpd/bin/envvars';
our $RequiredJavaVersion = '1.6';



sub new {
    my ($class, $prettyname) = @_;
    my $self = $class->SUPER::new();
    $self->{prettyname} = $prettyname;
    $self->{opensslver} = '0.9.8';
    $self->{openssltoonewver} = '0.9.9';
    return $self;
}

sub StartStopCommand {
    return '/etc/init.d/geserver';
}

sub SortPrimaryKey {
    return 20;
}

sub RequiredUsers {
    return ($Users::ApacheUser,
            $Users::TomcatUser,
            $Users::PostgresUser);
}

sub SetUsers {
    my $group = "gegroup";
    $group = prompt::GetString("Group name for all servers", $group);
    my $apacheuser = "geapacheuser";
    $apacheuser = prompt::GetString("Apache User", $apacheuser);
    my $tomcatuser = "getomcatuser";
    $tomcatuser = prompt::GetString("Tomcat User", $tomcatuser);
    my $pguser = "gepguser";
    $pguser = prompt::GetString("Postgres User", $pguser);
    $Users::ApacheUser = [$apacheuser, $group];
    $Users::TomcatUser = [$tomcatuser, $group];
    $Users::PostgresUser = [$pguser, $group];
}

sub PrimaryRPM {
    my $self = shift;
    return $ServerRPM;
}

sub IsRunning {
    return
        InstallUtils::ActivePidFile($ApachePidFile) ||
        InstallUtils::ActivePidFile($TomcatPidFile) ||
        InstallUtils::ActivePidFile($PostgresPidFile);
        
}

sub ExclusiveList {
    return ('GEServer', 'GEServerGoogle');
}

sub DeprecatesList {
    return ('KeyholeServer');
}

sub RPMList {
    my $self = shift;
    return ('GoogleEarthCommon',
            'gcc-ge-runtime',
            'expat-ge',
            'proj-ge',
            'openssl-ge',
            'openldap-ge',
            'apache-ge',
            'tomcat-ge',
            'geos-ge',
            'postgresql-ge',
            'postgis-ge',
            $ServerRPM,
            );
}

sub PreActionHandler {
    # Remove the webapp dirs manually. If tomcat did the right thing
    # (auto-update the ebapps from newer wars) then wouldn't have to do this.
    system("rm -Rf /opt/google/getomcat/webapps/SearchPublisher");
    system("rm -Rf /opt/google/getomcat/webapps/SearchServlet");
    system("rm -Rf /opt/google/getomcat/webapps/StreamPublisher");

    # tomcat can leave it's PID file around
    # an older version ran as root so the pidfile was owned by root
    # we know tomcat isn't running now, so clean the pidfile just in case
    unlink($TomcatPidFile);
}

sub CreateDirectories {
    # Create directories given as parameters if they don't already exist
    foreach my $dir (@_) {	
	if (! -d $dir )
	{
	    mkdir($dir, 755);
	}
    }
}

sub PreInstallHandler {
    # Disable stock apache if it exists
    # It can either be called 'apache' (on RH) or 'httpd' (on SuSe/SLES).
    # We can simply disable both
    foreach my $alt ('apache', 'apache2', 'httpd') {
        my $script = "/etc/init.d/$alt";
        if (-e $script) {
            print "Disabling $script ... ";
            system("$script stop >/dev/null 2>&1");
            InstallUtils::RemoveScript($alt);
        }
    }
}

sub SetServerFilePerms {

    my $tomcatusername = $Users::TomcatUser->[0];
    my $gegroup = $Users::TomcatUser->[1];

    for my $dir ('/opt/google/getomcat/webapps/StreamPublisher.war',
                 '/opt/google/getomcat/webapps/SearchPublisher.war',
                 '/opt/google/getomcat/webapps/SearchServlet.war',
                 '/opt/google/getomcat/lib/GEJniWrapper.jar',
                 '/opt/google/search/plugins/CoordinatePlugin.jar',
                 '/opt/google/search/plugins/ExamplePlugin.jar',
                 '/opt/google/search/plugins/GeocodingFederatedPlugin.jar',
                 '/opt/google/search/plugins/GEPlacesPlugin.jar',
                 '/opt/google/search/plugins/PoiPlugin.jar',
                 '/opt/google/search/plugins/GSAPlugin.jar',
                 '/opt/google/search/plugins/CoordinatePlugin.properties',
                 '/opt/google/search/plugins/ExamplePlugin.properties',
                 '/opt/google/search/plugins/GEPlacesPlugin.properties',
                 '/opt/google/search/plugins/GSAPlugin.properties',
                 '/opt/google/search/plugins/GeocodingFederatedPlugin.properties',
                 '/opt/google/search/plugins/POIPlugin.properties',
                 '/opt/google/search/plugins/postgres.properties',
                 '/opt/google/search/tabs/Coordinates.gestd',
                 '/opt/google/search/tabs/Example_Plugin.gestd',
                 '/opt/google/search/tabs/GeocodingFederated.gestd',
                 '/opt/google/search/tabs/Places.gestd',
                 '/opt/google/search/api/SearchAPI.jar') {
        if (-d $dir) {
            if (system("chown -R root:$gegroup $dir") != 0) {
                die "Unable to chown $dir\n";
            }
        }
    }
}

sub PostInstallHandler {
    # Post install steps are now included in the Installer script instead of
    # the RPM spec file.
    my $ge_root = "/opt/google";
    my $httpd_root = "$ge_root/gehttpd";
    my $tomcat_root = "$ge_root/getomcat";
    
    my $apacheusername = $Users::ApacheUser->[0];
    my $tomcatusername = $Users::TomcatUser->[0];
    my $pgusername = $Users::PostgresUser->[0];
    my $gegroup = $Users::ApacheUser->[1];

    # Replace IA_JAVA_HOME_VAR with JAVA_HOME value in geserver.
    InstallUtils::ReplaceTokenInFile(StartStopCommand(), 'IA_GEAPACHE_USER', $apacheusername);
    InstallUtils::ReplaceTokenInFile(StartStopCommand(), 'IA_GETOMCAT_USER', $tomcatusername);
    InstallUtils::ReplaceTokenInFile(StartStopCommand(), 'IA_GEPGUSER', $pgusername);

    # Write the users file into /etc/init.d/gevars.sh
    my $varsfile = '/etc/init.d/gevars.sh';
    open(VARSFILE, '>', $varsfile) || die "Unable to create $varsfile: $!\n";
    print VARSFILE "GEAPACHEUSER=$apacheusername\n";
    print VARSFILE "GETOMCATUSER=$tomcatusername\n";
    print VARSFILE "GEPGUSER=$pgusername\n";
    print VARSFILE "GEGROUP=$gegroup\n";
    close(VARSFILE);
    
    InstallUtils::systemordie("mkdir -p $httpd_root/conf.d/virtual_servers/runtime");
    InstallUtils::systemordie("touch $httpd_root/conf.d/virtual_servers/runtime/default_ge_runtime");
    InstallUtils::systemordie("touch $httpd_root/conf.d/virtual_servers/runtime/default_map_runtime");
    InstallUtils::systemordie("chown -R $apacheusername:$gegroup $httpd_root/conf.d/virtual_servers/runtime");
    InstallUtils::systemordie("chmod -R 775 $httpd_root/conf.d/virtual_servers/runtime");
    SetServerFilePerms();

    # Remove the old location for postgres database.
    system("rm -Rf $ge_root/share/pgsql");
    
    # Make sure needed directories exist(some .rpms are not correctly
    # imported into .debs, and some of the directories seem to not get created.
    CreateDirectories("$tomcat_root/temp", "$tomcat_root/webapps", "$tomcat_root/work", "/var/opt/google/pgsql", "/var/opt/google/log" );

    # change the ownership of the postgres directory because postgres needs to write to it
    InstallUtils::systemordie("chown -R $pgusername:$gegroup /var/opt/google/pgsql");

    # Make getomcatuser the owner of certain tomcat dirs coz it needs write
    # to those dirs.
    InstallUtils::systemordie("chown -R $tomcatusername:$gegroup $tomcat_root/logs $tomcat_root/temp $tomcat_root/webapps $tomcat_root/work");

    # geresetpgdb will now be called only if pg data dir does not exist.
    my $pg_data_dir = '/var/opt/google/pgsql/data';
    if (! -e $pg_data_dir) {
      print "\nCreating postgres db...\n";
      if (system("cd / ; sudo -u $pgusername $ge_root/bin/geresetpgdb") != 0) {
          die "Unable to create postgres db\n";
      }
    }
    
    # Prompt for publishroot location
    if (system("$ge_root/bin/geconfigurepublishroot") != 0) {
        die "Unable to create publish root.\n";
    }
    
    InstallUtils::InstallScriptOrDie("geserver");
}


sub ExtractPreviousJavaHome {
    my ($self) = @_;
    my $envfile = $TomcatEnvFile;
    if (-f $envfile) {
        open(ENVFILE, '<', $envfile) || die "Unable to open $envfile: $!\n";
        while (<ENVFILE>) {
            chomp;
            if (/JAVA_HOME=(.+)/) {
                close(ENVFILE);
                return $1;
            }
        }
        close(ENVFILE);
    }
    return undef;
}

sub SetJavaHome {
    my ($self, $java_home) = @_;
    
    my $envfile = $TomcatEnvFile;
    open(ENVFILE, '>', $envfile) || die "Unable to open $envfile: $!\n";
    print ENVFILE "export JAVA_HOME=$java_home\n";
    print ENVFILE "export JRE_HOME=$java_home\n";
    print ENVFILE "unset  JAVA_OPTS\n";
    print ENVFILE "export CATALINA_PID=$TomcatPidFile\n";
    close(ENVFILE);

    # Replace IA_JAVA_HOME_VAR with JAVA_HOME value in geserver.
    InstallUtils::ReplaceTokenInFile(StartStopCommand(), 'IA_JAVA_HOME_VAR', $java_home);
}


sub FindJavaHome {
    my ($self, $disallow_hash) = @_;
    
    my $java_home = $self->ExtractPreviousJavaHome();

    if (defined($java_home) &&
        !$self->ValidateJavaHome($java_home, $disallow_hash, 0)) {
        undef $java_home;
    }

    TermHelps::Clear();
    $java_home = $self->PromptUserForJavaHome($java_home, $disallow_hash);
    return $java_home;
}

sub ValidateJavaHome {
    my ($self, $java_home, $disallow_hash, $emitmsg) = @_;

    if (defined($disallow_hash) && exists $disallow_hash->{$java_home}) {
        warn "You may not specify $java_home.\n" if $emitmsg;
        return 0;
    }

    my $java_path = "$java_home/bin/java";

    if (! -f $java_path) {
        warn "Cannot find java under $java_home.\n" if $emitmsg;
        return 0;
    }

    # We only care about the major and minor version
    my $output=`$java_path -version 2>&1`;
    if ($output !~ /java version "(\d\.\d)\..*"/) {
        warn "Cannot detect java version for $java_path.\n" if $emitmsg;
        return 0;
    }
    my $ver = $1;
    my $req = $RequiredJavaVersion;

    if ($ver ne $req) {
        warn "$java_path is wrong version ($ver). Must be $req.\n" if $emitmsg;
        return 0;
    }

    return 1;
}

sub PromptUserForJavaHome {
    my ($self, $default, $disallow_hash) = @_;
    my $java_home;

    print "Google Earth Enterprise Server requires Java Runtime Environment\n",
          "version $RequiredJavaVersion.\n\n";

    my $prompt = "Please enter directory for \$JAVA_HOME";
    
    TermHelps::PushWarnState();
    while (1) {
        $java_home = prompt::GetDirectory($prompt, $default);
        if ($self->ValidateJavaHome($java_home, $disallow_hash, 1)) {
            last;
        } else {
            TermHelps::Beep();
        }
    }
    TermHelps::PopWarnState();
    return $java_home;
}



1;
