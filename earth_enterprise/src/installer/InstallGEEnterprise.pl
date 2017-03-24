#!/usr/bin/perl -w-
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

use strict;
use FindBin;
use lib "$FindBin::Bin/lib";
BEGIN {
  our $ProductDir = "$FindBin::Bin/products"
}
use TermHelps;
use BootstrapAllProducts;
use Product;
use ProductSelection;
use NeededUsers;
use RPMVersion;
use Term::ANSIColor qw(:constants);
use Getopt::Long;

# do some additional sanity checks
BootstrapAllProducts::ExtraChecks();

CheckHostname();

# make sure we're running as root
die "You must run as root\n" unless $> == 0;

$| = 1;

my $usage = <<EOF;
usage: $FindBin::Script [options]
Valid options are:
    --help, -?:            Display this help message
    --platform <platform>: Use specified platform [default auto]
    --skipusers:           Skip the checking and fixing or users/groups
    --verbose:             Show more information
    --debug:               Show a lot of information
EOF


my $help = 0;
our $platform;
my $verbosity = 0;
my $skipusers = 0;
GetOptions('help|?'     => \$help,
           'platform=s' => \$platform,
           'skipusers'  => \$skipusers,
	   'verbose'    => sub { $verbosity = 1 },
	   'debug'      => sub { $verbosity = 2 },
	   )
    || die $usage;
if ($help) {
    print $usage;
    exit(0);
}
if ($verbosity > 0) {
    $TermHelps::WARN_EMITTED = 1;
}

# make sure our machine is compatible with this package
$platform = FindInstallPlatformOrDie($platform);

# on ubuntu platform we need to install the capabilities module in order to continue installation
if ($platform eq 'ubuntu') {
    InstallCapabilityModule();
}

# make the package management system aware of which system to use .rpm or .deb
RPMHelps::SetPlatform($platform);

print "Installing for platform $platform ...\n";
my $platform_rpms = RPMHelps::ReadRPMsFromDirs("$FindBin::Bin/RPMS/noarch",
                                               "$FindBin::Bin/RPMS/$platform");

# make sure it's OK for us to run
CheckForRunning();
{
    my $badcount = 0;
    foreach my $product (@Product::AllProductList) {
        if ($product->IsInstalled() &&
            !$product->OKToRunInstaller()) {
            # warning already emitted
            ++$badcount;
        }
    }
    if ($badcount > 0) {
        die "Unable to proceed.\n";
    }
}

# print "========== Debug Dump ==========\n";
# BootstrapAllProducts::DumpProductInfo();
# print "================================\n";

my $something_installed = 0;
my $something_uninstalled = 0;
my $something_upgraded = 0;

{
# get and confirm the users product selection
    my $prodsel = ProductSelection::GuessInitial();
    my $actions;
    do {
        $prodsel = ProductSelection::PromptUser($prodsel);
        $actions = CalculateActions($prodsel);
        $something_installed = 0;
        $something_uninstalled = 0;
        $something_upgraded = 0;
    } while (!ConfirmActions($actions));


# make sure we have all the users needed by our selected products
    if (!$skipusers) {
	if (prompt::Confirm("Would you like to change the default user and group names", "N")) {
            foreach my $prodname (keys %{$prodsel}) {
               my $product = $Product::AllProductHash{$prodname};
               if ($product->can("SetUsers")) {
                   $product->SetUsers();
               }
            }
        }
        my $needed = new NeededUsers($prodsel);
        if (!$needed->Empty()) {
            $needed->FixOrDie($prodsel);
        }
    }

    CallPreActions($actions);

    DoRPMMagic($actions);

    CallPostActions($actions);

    print "Verifying compatibility of system libraries ...\n";
    if (system("perl $FindBin::Bin/lib/ValidateLibs.pl") != 0) {
        die "Incompatible system libraries found\n";
    }

    UpdateInstalledInfo($actions);

    TermHelps::Clear();
    if ($something_uninstalled) {
      print "Uninstall SUCCESSFUL.\n";
    }
    if ($something_upgraded) {
      print "Upgrade SUCCESSFUL.\n";
    }
    if ($something_installed) {
      print "Install SUCCESSFUL.\n";
    }
    if ($something_upgraded || $something_installed) {
      print "Please start your installed products.\n";
      foreach my $product (@Product::AllProductList) {
        if ($product->AvailableInPackage() &&
            $product->IsInstalled(1)) {
          my $cmd = $product->StartStopCommand();
          if (defined ($cmd)) {
            print "    $cmd start\n";
          }
        }
      }
      print "\n";
      print "If this is your first installation, please logout and\n";
      print "log back in to update your PATH to include /opt/google/bin.\n";
      print "\n";
    }
}

sub InstallCapabilityModule {
    `modprobe capability`;
    open(MOD, '/etc/modules');
    my $ret = grep(/capability/, <MOD>);
    close(MOD);
    if ($ret==0)
    {
	`echo capability >> /etc/modules`
    } 
}


sub CheckForRunning {
    my $first = 1;
    foreach my $product (@Product::AllProductList) {
        if ($product->IsInstalled() && $product->IsRunning()) {
            if ($first) {
                $first = 0;
                warn "Some Google Earth Enterprise products are still running.\n";
                warn "Please run the following to stop them:\n"
            }
            my $cmd = $product->StartStopCommand();
            if (defined $cmd) {
                warn "    $cmd stop\n";
            }
        }
    }
    if (!$first) {
        die "\n";
    }
}

sub UpdateInstalledInfo {
    my $actions = shift;
    foreach my $product (@{$actions->{uninstall}}) {
        $product->EraseInstalledInfo();
    }
    foreach my $product (@{$actions->{installorupgrade}}) {
        $product->WriteInstalledInfo();
    }
}


sub CalculateActions {
    my $prodsel = shift;

    my $actions = {
        all          => [],
        uninstall    => [],
        upgrade      => [],
        install      => [],

        # intentionally not in 'all'
        noopinstall  => [],
    };
    foreach my $product (@Product::AllProductList) {
        my $prodname  = ref($product);
        my $selected  = exists $prodsel->{$prodname};
        my $installed = $product->IsInstalled();

        if ($installed) {
            if (!$selected) {
                # want uninstall
                push @{$actions->{all}}, $product;
                push @{$actions->{uninstall}}, $product;
            } elsif (RPMVersion::Compare
                     ($product->AvailableVersion(),
                      $product->InstalledVersion()) > 0) {
                # want upgrade
                push @{$actions->{all}}, $product;
                push @{$actions->{upgrade}}, $product;
                push @{$actions->{installorupgrade}}, $product;
            } elsif (RPMVersion::Compare
                     ($product->AvailableVersion(),
                      $product->InstalledVersion()) == 0) {
                # intentionally not in 'all'
                push @{$actions->{noopinstall}}, $product;
            }
        } elsif ($selected) {
            # want install
            push @{$actions->{all}}, $product;
            push @{$actions->{install}}, $product;
            push @{$actions->{installorupgrade}}, $product;
        }
    }
    return $actions;
}

sub ConfirmActions {
    my $actions = shift;
    my $something = 0;

    TermHelps::Clear();
    print <<EOF;
Based on your product selection, the following actions need to be taken:

EOF
    
    if (@{$actions->{uninstall}}) {
        $something = 1;
        $something_uninstalled = 1;
        print "Uninstall:\n";
        foreach my $product (@{$actions->{uninstall}}) {
            my $pretty = $product->PrettyName();
            my $ver = $product->InstalledVersion();
            print "   $pretty -- ($ver)\n";
        }
    }
    if (@{$actions->{upgrade}}) {
        $something = 1;
        $something_upgraded = 1;
        print "Upgrade:\n";
        foreach my $product (@{$actions->{upgrade}}) {
            my $pretty = $product->PrettyName();
            my $instver = $product->InstalledVersion();
            my $availver = $product->AvailableVersion();
            print "   $pretty -- ($instver -> $availver)\n";
        }
    }
    if (@{$actions->{install}}) {
        $something = 1;
        $something_installed = 1;
        print "Install:\n";
        foreach my $product (@{$actions->{install}}) {
            my $pretty = $product->PrettyName();
            my $ver = $product->AvailableVersion();
            print "   $pretty -- ($ver)\n";
        }
    }
    if (!$something) {
        print "No products to install/upgrade/uninstall.\n";
        print "Additional checks will still be made.\n";
    }
    print "\n";

    my $confirm = prompt::Confirm("Perform these actions", 'Y');
    print "\n";

    return $confirm;
}

sub CallPreActions {
    my $actions = shift;

    foreach my $product (@{$actions->{all}}) {
        $product->PreActionHandler();
    }
    foreach my $product (@{$actions->{uninstall}}) {
        $product->PreUninstallHandler();
    }
    foreach my $product (@{$actions->{installorupgrade}}) {
        $product->PreInstallHandler();
    }
}

sub CallPostActions {
    my $actions = shift;

    # call in reverse order from Pre
    foreach my $product (@{$actions->{installorupgrade}}) {
        $product->PostInstallHandler();
    }
    foreach my $product (@{$actions->{uninstall}}) {
        $product->PostUninstallHandler();
    }
    foreach my $product (@{$actions->{all}}) {
        $product->PostActionHandler();
    }
}

sub DoRPMMagic {
    my $actions = shift;

    my ($want, $dontwant) = GatherRPMLists($actions);

    if (@{$dontwant}) {
        RPMHelps::EraseRPMS($dontwant, $verbosity);

        # re-gather after the first erase in case we didn't get them all
        ($want, $dontwant) = GatherRPMLists($actions);
    }

    if (@{$want}) {
        RPMHelps::UpgradeRPMS($want, $verbosity);
    }

    if (@{$dontwant}) {
        RPMHelps::EraseRPMS($dontwant, $verbosity);

        # re-gather after the first erase in case we didn't get them all
        ($want, $dontwant) = GatherRPMLists($actions);
        if (@{$dontwant}) {
            warn "Unable to uninstall all old RPMs. We'll just leave them around.\n";
            my @rpmnames = map($_->{name}, @{$dontwant});
            foreach my $rpmname (@rpmnames) {
                warn "   $rpmname\n";
            }
	    my $output;
	    if ($platform eq "ubuntu") {
		$output = `dpkg --purge @rpmnames 2>&1`;
	    } else {
	        $output = `rpm -e @rpmnames 2>&1`;
	    }
            warn $output;
        }
    }
}



sub FindInstallPlatformOrDie {
    my $platform = shift;
    if (!defined $platform) {
        $platform = InstallUtils::GetPlatform();
    }
    my @platforms = ( $platform );
    if ($platform eq 'x86_64') {
	push @platforms, 'i686';
    }
 
    foreach my $p (@platforms) {
	if (-d "$FindBin::Bin/RPMS/$p") {
	    return $p;
	}
    }


    die "Unable to find RPMs compatible with platform '$platform'.\n";
}

sub GatherRPMLists {
    my $actions = shift;
    { RPMHelps::ClearInstalledVersionCache(); }

      
    # populate the basic lists
    my %want;
    my %dontwant;
    foreach my $product (@{$actions->{noopinstall}}) {
        foreach my $rpmname ($product->RPMList()) {
            $want{$rpmname} = 1;
        }
    }
    foreach my $product (@{$actions->{install}}) {
        foreach my $rpmname ($product->RPMList()) {
            $want{$rpmname} = 1;
        }
    }
    foreach my $product (@{$actions->{upgrade}}) {
        foreach my $rpmname ($product->RPMList()) {
            $want{$rpmname} = 1;
        }
        my $info = $product->InstalledInfo();
        foreach my $rpmname (@{$info->{rpms}}) {
            if (!exists $want{$rpmname}) {
                $dontwant{$rpmname} = 1;
            }
        }
    }
    foreach my $product (@{$actions->{uninstall}}) {
        my $info = $product->InstalledInfo();
        foreach my $rpmname (@{$info->{rpms}}) {
            if (!exists $want{$rpmname}) {
                $dontwant{$rpmname} = 1;
            }
        }
    }
    
    # build real list of dontwants
    # Eliminate those that aren't installed
    my @dontwant;
    foreach my $rpmname (keys %dontwant) {
        my $inst_verref = RPMHelps::InstalledVersionRef($rpmname);
        if (defined $inst_verref) {
            push @dontwant, $inst_verref;
        }
    }


    # build real list of want
    # eliminate those that are already up to date
    my @want;
    my @badarch;
    foreach my $rpmname (keys %want) {
        my $inst_verref = RPMHelps::InstalledVersionRef($rpmname);
        my $avail_verref;
        $avail_verref = $platform_rpms->{$rpmname};
        if (!defined $avail_verref) {
            $avail_verref = $platform_rpms->{lc($rpmname)};
            if (!defined $avail_verref) {
                die "PACKAGING ERROR: Missing RPM '$rpmname'\n";
            }
        }
        if (!defined($inst_verref)) {
            push @want, $avail_verref;
        } elsif (RPMVersion::Compare($inst_verref, $avail_verref) < 0) {
            my $iver = $inst_verref->{fullver};
            my $aver = $avail_verref->{fullver};
            push @want, $avail_verref;
        } elsif ($inst_verref->{arch} ne $avail_verref->{arch}) {
            push @badarch, [$inst_verref, $avail_verref];
        }
    }

    if (@badarch) {
        {
            warn <<EOF;
The following installed RPMs have a different architecture than the
one that is availble to install:
    INSTALLED  AVAILABLE
EOF
        }
        foreach my $rpm (@badarch) {
            my $name  = $rpm->[0]{fullname};
            my $iarch = $rpm->[0]{arch};
            my $aarch = $rpm->[1]{arch};
            warn("    ",
                  $iarch, ' 'x(11-length($iarch)),
                  $aarch, ' 'x(11-length($aarch)),
                  $name, "\n");
        }
        die "Unable to proceed\n";
    }

    return ([ @want ], [ @dontwant ]);
}


sub CheckHostname {
    my $hostname = `hostname`;
    chomp $hostname;

    if (($hostname eq '(none)') ||
        ($hostname eq 'linux') ||
        ($hostname eq 'localhost') ||
        ($hostname =~ /^localhost\./)) {
        warn <<EOF;
Google Earth Enterprise requires each machine to have a properly configured
hostname. This machine's hostname is currently defined as
'$hostname'.
That name looks like a default setting left over from a fresh installation.
EOF
        if (!prompt::Confirm("Is '$hostname' the correct name for this machine", 'N')) {
            die "Please configure the hostname for this machine and re-run the installer\n";
        }
    }

    if (($hostname =~ /dhcp/i) ||
        ($hostname =~ /bootp/i)) {
        warn <<EOF;
Google Earth Enterprise requires each machine to have a hostname that never
changes. This machine's hostname is currently defined as
'$hostname'.
That name looks like it has been assigned by a network boot server. Such names
are typically subject to change over time. That would break this software.
EOF
        if (!prompt::Confirm("Is '$hostname' the correct name for this machine", 'N')) {
            die "Please configure the hostname for this machine and re-run the installer\n";
        }
    }


    if (!gethostbyname($hostname)) {
        die <<EOF;
This machine's hostname ($hostname) does not map to an IP address.
Please configure the hostname for this machine and re-run the installer
EOF
    }
}
