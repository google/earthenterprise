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

package GEFusion;

use strict;
use Product;
use Users;
use RPMHelps;
use File::Path;
use prompt;
use TermHelps;

our( @ISA );
@ISA = ("Product");

# ****************************************************************************
# ***  Base class used by GEFsionLT, GEFusionPro, & GEFusionGoogle products
# ****************************************************************************

sub new {
    my ($class, $prettyname, $fusionrpm) = @_;
    my $self = $class->SUPER::new();
    $self->{prettyname} = $prettyname;
    $self->{fusionrpm} = $fusionrpm;
    return $self;
}

sub StartStopCommand {
    return '/etc/init.d/gefusion';
}

sub SortPrimaryKey {
    # make it near the front, but leave some room to make it easier to add
    # something else in front
    return 10;
}

sub RequiredUsers {
    return ($Users::FusionUser);
}

sub SetUsers {
    my $user = "gefusionuser";
    $user = prompt::GetString("Fusion User", $user);
    my $group = "gegroup";
    $group = prompt::GetString("Fusion Group", $group);
    $Users::FusionUser = [$user, $group];
}

sub PrimaryRPM {
    my $self = shift;
    return $self->{fusionrpm};
}

sub IsRunning {
    if (-x '/opt/google/bin/gefdaemoncheck') {
        return ((system('/opt/google/bin/gefdaemoncheck', '--checkrunning')==0) ? 1 : 0);
    } else {
        return 0;
    }
}

sub ExclusiveList {
    return ('GEFusionLT', 'GEFusionPro', 'GEFusionGoogle',
             'GEServerDisconnected');
}

sub DeprecatesList {
    return ('KeyholeFusionLT', 'KeyholeFusionPro', 'KeyholeFusionGoogle');
}

sub RPMList {
    my $self = shift;
    return ('GoogleEarthCommon',
            'gcc-ge-runtime',
            'expat-ge',
            'qt-ge',
            'xerces-c-ge',
            'libtiff-ge',
            'proj-ge',
            'libgeotiff-ge',
            'ogdi-ge',
            'gdal-ge',
            'libcurl-ge',
            'libjs-ge',
            'libsgl-ge',
            'geos-ge',
            $self->{fusionrpm},
            );
}

sub OKToRunInstaller {
    my $assetroot = GetAssetRoot();
    if (defined $assetroot) {
        if (glob("$assetroot/.state/*.task")) {
            warn "Pending fusion tasks exist!\n" .
                "Please cancel them or let them finish before running the installer.\n";
            return 0
        }
    }
    return 1;
}

sub PreInstallHandler {
    PreCleanup();
}

sub PreCleanup {
    # clean up poorly created directory in earlier version of RPM
    my $etclink = '/opt/google/etc';
    if (-d $etclink && ! -l $etclink) {
        rmtree($etclink);
        symlink('/etc/opt/google', $etclink);
    }

    # fixup perms from early development release of Fusion 3.0
    my $groupname = $Users::FusionUser->[1];
    for my $dir ('/var/opt/google/log',
                 '/var/opt/google/run') {
        if (-d $dir) {
            if (system("chown -R root:$groupname $dir") != 0) {
                die "Unable to chown $dir\n";
            }
        }
    }
}

sub PostInstallHandler {
    my $assetroot = GetAssetRoot();
    if (!$assetroot) {
        $assetroot = PromptAssetRoot();
    }

    # fixup perms based on usernames
    my $groupname = $Users::FusionUser->[1];
    for my $dir ('/var/opt/google/log',
                 '/var/opt/google/run') {
        if (-d $dir) {
            if (system("chown -R root:$groupname $dir") != 0) {
                die "Unable to chown $dir\n";
            }
        }
    }

    # Create a SystemRC file for Fusion with the specified users/groups.
    my $user = $Users::FusionUser->[0];
    my $group = $Users::FusionUser->[1];
    SetSystemRCUserAndGroup($user, $group);

    if (-d $assetroot) {
        InstallUtils::systemordie('/opt/google/bin/geupgradeassetroot',
                                  "--assetroot", $assetroot);
    } else {
       InstallUtils::systemordie("/opt/google/bin/geconfigureassetroot",
                                 "--new", "--assetroot", $assetroot);
    }
    InstallUtils::systemordie("/opt/google/bin/geselectassetroot",
                              "--assetroot", $assetroot);
    InstallUtils::InstallScriptOrDie("gefusion");
}

my $rcfile = "/etc/opt/google/systemrc";

# Update the systemrc file with username and groupname.
sub SetSystemRCUserAndGroup($$) {
   my $user = shift @_;
   my $group = shift @_;
   my $contents = '';
   # 2 cases: prexisting systemrc or no systemrc
   if (-e $rcfile) {
      # Add the fusionUser and group names to the rc file, replacing
      # the previous ones if they exist.
      open(FILE, "< $rcfile") or die "Unable to open $rcfile\n";
      while(<FILE>) {
         # Don't output the user or group name or the Systemrc delimiter.
         # We'll append those to the end of everything else.
         $contents .= $_ if (/[a-zA-Z]+/ && !(/fusionUsername/ || /userGroupname/ || /<\/Systemrc>/));
      }
      close(FILE);
   } else {
      # Create the header for the xml systemrc file
      $contents = <<EOF
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<Systemrc>
EOF
   }

   # append the username and group name to the end of the xml systemrc file.
   $contents .= <<EOF;
  <fusionUsername>$user</fusionUsername>
  <userGroupname>$group</userGroupname>
</Systemrc>
EOF

   open(FILE, "> $rcfile") or die "Unable to create $rcfile\n";
   print FILE $contents;
   close(FILE);
   # Must set the ownership properly.
   if (system("chown -R $user:$group $rcfile") != 0) {
       die "Unable to chown $rcfile\n";
   }
}

sub GetAssetRoot
{
    if (!open (SYSTEMRC, "$rcfile")) {
        return undef;
    }
    while (<SYSTEMRC>) {
        if (/<assetroot>(.*)<\/assetroot>/) {
	    close SYSTEMRC;
	    return $1;
        }
    }
    close SYSTEMRC;
    return undef;
}


sub PromptAssetRoot
{
    {
        TermHelps::Clear();
          print <<EOF;
Google Earth Enterprise Fusion needs to have an assetroot configured.
No previous assetroot was detected.

EOF
    }
    my $assetroot = '/gevol/assets';
    TermHelps::PushWarnState();
    while (1) {
        $assetroot = prompt::GetDirectory("Please enter assetroot",
                                          $assetroot,
                                          1);  # 1 -> allowmissing
        if (-d $assetroot) {
            if (-f "$assetroot/.config/.fusionversion") {
                print "$assetroot looks like an assetroot.\n";
                if (prompt::Confirm("Would you like to upgrade it to work with this version of fusion", 'Y')) {
                    last;
                }
            } else {
                warn "$assetroot already exists.\n";
                warn "It does not appear to be an assetroot.\n";
                warn("If you really want to use this path, ",
                     "please remove the directory and try again.\n");
            }
        } else {
            last;
        }
    }
    TermHelps::PopWarnState();

    return $assetroot;
}




1;
