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

package Product;

use strict;
use warnings;
use Data::Dumper;
use File::Path;

# Will be populated by all of the product classes
our ( @AllProductList, %AllProductHash );

my $InstalledProductsPath = '/etc/opt/google/installed_products';

sub new {
    my ($class) = @_;
    return bless { }, $class;
}

sub SortPrimaryKey {
    # used as primary key for sorting products
    # sort order affects the order the products appear in the selection menu
    # more importantly, sort order affects the order that scripts are run
    return 100;
}

sub PrettyName {
    my $self = shift;
    if (exists $self->{prettyname}) {
        return $self->{prettyname};
    } else {
        return ref($self);
    }
}

sub IsDeprecated {
    return 0;
}

sub StartStopCommand {
    return undef;
}

sub RequiredUsers {
    return ();
}


sub OKToRunInstaller {
    # called before any action is attempted
    # this is a good place to put checks for things not handled automatically
    # by the system.

    # func should "warn" and return 0 if not safe to perform any actions
    # otherwise should be silent and return 1

    return 1;
}

sub IsRunning {
    return 0;
}

sub DependsList {
    # return list of other products that I depend on
    return ();
}

sub ExclusiveList {
    # return list of other products that cannot be packaged with me
    return ();
}

sub InstallExclusiveList {
    my $self = shift;
    # return list of other products that cannot be installed with me
    return $self->ExclusiveList();
}

sub DeprecatesList {
    # return list of other products that this product will deprecate
    return ();
}

sub RPMList {
    # returns list of RPMs that need to be installed for this product
    # NOTE: these are just the simple name (e.g expat-ge)
    # NOTE any rpms referenced here must be in packager/rpminfo.pm
    return ();
}


# called _before_ any other Handlers or RPM work
# all products with an action to perform will have this called
sub PreActionHandler {
}
                
# called _before_ any RPM work happens
# all products with an action to perform will have exactly one of these called
# These are processed in batches (all uninstalls, then installs)
# the install handler is called for both upgrades and installs
sub PreUninstallHandler {
}
sub PreInstallHandler {
}

# called _after_ any RPM work happens
# all products with an action to perform will have exactly one of these called
# These are processed in batches (all installs, then uninstalls)
# the install handler is called for both upgrades and installs
# NOTE: this is the inverse order from the Pre's
sub PostInstallHandler {
}
sub PostUninstallHandler {
}

# called _after_ any other Handlers or RPM work
# all products with an action to perform will have this called
sub PostActionHandler {
}



########## functions below here are not to be specialized ##########
sub InstalledInfoFilename {
    my $self = shift;
    my $prodname = ref($self);
    return "$InstalledProductsPath/$prodname.product";
}

sub LoadOrBootstrapInstalledInfo {
    my $self = shift;

    my $prodinfo;
    my $prodfile = $self->InstalledInfoFilename();
    if (-f $prodfile) {
        open(PRODFILE, $prodfile) || die "Unable to open $prodfile: $!\n";
        my $dump;
        {
            local $/ = undef;
            $dump = <PRODFILE>;
            $prodinfo = eval $dump;
        }
        close(PRODFILE);

        if (!defined $prodinfo) {
            print "-----\n";
            print $dump;
            print "-----\n";
            die "Unable to interpret $prodfile: $@\n";
        }
    } elsif ($self->UNIVERSAL::can('QueryInstalledVersion')) {
        my $ver = $self->QueryInstalledVersion();
        if (defined $ver) {
            $prodinfo = $self->WriteInstalledInfo($ver);
        }
    }
    if (!defined $prodinfo) {
        $prodinfo = {
            ver => undef,
            rpms => [],
        };
    }
    $self->{installed_info} = $prodinfo;
}

sub WriteInstalledInfo {
    my ($self, $ver) = @_;
    if (!defined $ver) {
        $ver = $self->AvailableVersion();
    }
    my $rpms = [ $self->RPMList() ];

    my $prodinfo = { ver => $ver,
                     rpms => $rpms };
    my $prodfile = $self->InstalledInfoFilename();
    mkpath($InstalledProductsPath, 0, 0755);
    open(PRODFILE, '>', $prodfile) ||
        die "Unable to open $prodfile for writing: $!\n";
    $Data::Dumper::Terse  = 1;
    print PRODFILE Dumper($prodinfo);
    close(PRODFILE);

    return $prodinfo;
}

sub EraseInstalledInfo {
    my $self = shift;
    my $prodfile = $self->InstalledInfoFilename();
    unlink($prodfile);
}

sub InstalledInfo {
    my ($self, $skipcache) = @_;
    if ($skipcache || !exists $self->{installed_info}) {
        $self->LoadOrBootstrapInstalledInfo();
    }
    return $self->{installed_info};
}

sub InstalledVersion {
    my ($self, $skipcache) = @_;
    my $info = $self->InstalledInfo($skipcache);
    return $info->{ver};
}

sub IsInstalled {
    my ($self, $skipcache) = @_;
    return defined $self->InstalledVersion($skipcache);
}

sub AvailableInPackage {
    return 0;
}

1;
