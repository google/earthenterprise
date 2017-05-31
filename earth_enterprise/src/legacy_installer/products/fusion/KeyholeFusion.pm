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

package KeyholeFusion;

use strict;
use Product;
use Users;
use RPMHelps;
use InstallUtils;

our( @ISA );
@ISA = ("Product");

# ****************************************************************************
# ***  Base class used by KeyholeFusionLT, KeyholeFusionPro, &
# ***  KeyholeFusionGoogle products
# ****************************************************************************

sub new {
    my ($class, $prettyname, $fusionrpm) = @_;
    my $self = $class->SUPER::new();
    $self->{prettyname} = $prettyname;
    $self->{fusionrpm} = $fusionrpm;
    return $self;
}

sub StartStopCommand {
    return '/etc/init.d/khsystem';
}

sub IsDeprecated {
    return 1;
}

sub QueryInstalledVersion {
    my $self = shift;
    return RPMHelps::InstalledVersion($self->{fusionrpm});
}

sub IsRunning {
    return
        InstallUtils::ActivePidFile('/usr/keyhole/run/khsystemmanager.pid') ||
        InstallUtils::ActivePidFile('/usr/keyhole/run/khresourceprovider.pid');
}

sub RPMList {
    my $self = shift;

    return ('KeyholeCommon',
            'gdal-keyhole',
            'libgeotiff-keyhole',
            'libtiff-keyhole',
            'ogdi-keyhole',
            'proj-keyhole.rpm',
            'qt-keyhole',
            'expat-keyhole',
            'xerces-c-keyhole',
            'libjs-keyhole',
            'libcurl-keyhole',
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


sub PostUninstallHandler {
    warn "XXX - Implement me";
}


sub GetAssetRoot
{
    my $rcfile = "/usr/keyhole/etc/systemrc";
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


1;
