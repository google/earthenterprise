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
use Getopt::Long;
use File::Path;
use File::Basename;
use File::Copy;
use FindBin;
use lib "$FindBin::Bin";
use lib "$FindBin::Bin/../installer/lib";
BEGIN {
  our $ProductDir = "$FindBin::Bin/../installer/products"
}
use platforms;
use packages;
use rpminfo;
use BootstrapAllProducts;


$| = 1;


my $help = 0;
my $verbose = 0;
my $user = getpwuid($<);
my $gepackagedir = "";
my $skipsync = 0;
my $is_local = 0;
my $overwrite = 0;
my $distver;
my $only_packname = "";

sub usage
{
    die <<EOF
$FindBin::Script --packagedir <dir> [options]

Required arguments:
    --packagedir <dir>:    Where to write [required]
Supported options:
    --help:                Display this usage message
    --local <source_packages_dir>      
                           Build the installer package locally for a single architecture
                           using rpms from the specified directory.
                           e.g., .../packages   where packages/RPMS and packages/SRPMS 
                           contain the latest RPMS and SRPMS
    --package              one of pro | google | lt, to only build a single package
    --verbose:             Display progress information
    --skipsync:            Skip rsync of RMS from build machines
    --overwrite            Overwrite existing dist dir
$FindBin::Script gathers all the files necessary to make Google Earth
Enterprise distribution CD images. The contents of the CD images are controlled
packages.pm in this directory and all the .product files underneath
src/installer/products.
EOF
}


# get commandline arguments
GetOptions('help|?'    => \$help,
           'verbose'   => \$verbose,
           'local'  => \$is_local,
           'skipsync'  => \$skipsync,
           'overwrite' => \$overwrite,
           'packagedir=s' => \$gepackagedir,
           'package=s' => \$only_packname,
           ) or usage();
usage() if $help || $gepackagedir eq "";

my $gedistdir = "$gepackagedir/dist";
my $gebuilddir = "../../rpms/packages/RPMS";

# Check if we're locally packaging.
my $local_arch = 'i686';
$skipsync = 1 if $is_local;
$local_arch = 'x86_64' if (`uname -m` =~ /x86_64/);

if ($is_local) {
      print "Local Packaging for $local_arch'\n";
} elsif (! $skipsync) {
     # rsync the packages from the build machines
    print "\n========== Rsyncing RPMS from build machines ==========\n";
    foreach my $platform (@platforms::LIST) {
        my $platformname = $platform->{name};
        my $v = $verbose ? 'v' : '';
        my $rpmdir = $platform->{rpmdir};
        my $destdir = $gebuilddir;
        if (! -d $destdir) {
            assertsystem("mkdir -p $destdir");
        }
        assertsystem("rsync -rt$v $rpmdir $destdir");
    }
} else {
    print "Skipping packages rsync.\n";
}

print "\n========== Check for required RPM versions ==========\n";
# check the noarch RPMs
my $SomeMissing = 0;
my %RPMNoArchFiles;
{
    my ($rpmname, $rpminfo);
    while (($rpmname, $rpminfo) = each %rpminfo::NOARCH) {
        my $version = $rpminfo->{ver};
        foreach my $platform (@platforms::LIST) {
            my $platformname = $platform->{name};
            my $arch = $platform->{arch};
            # Skip this if we're doing a local package for a different architecture.
            next if $is_local && $arch ne $local_arch;

            my $destdir = "$gebuilddir/noarch";
            my $rpmfile = "$destdir/$rpmname-$version.noarch.rpm";
            if (-f $rpmfile) {
                $RPMNoArchFiles{$rpmname} = $rpmfile;
                last;
            }
        }
        if (! exists $RPMNoArchFiles{$rpmname} ) {
            warn "Unable to find $rpmname-$version.noarch.rpm\n";
            $SomeMissing = 1;
        }
    }
}
# check the arch specific RPMs
my %RPMArchFiles;
{
    my ($rpmname, $rpminfo);
    while (($rpmname, $rpminfo) = each %rpminfo::ARCH) {
        my $version = $rpminfo->{ver};
        if ($rpmname eq 'GoogleEarthCommon') {
        #   ($distver) = $version =~ /^([^-]+)/;
            ($distver) = $version;
        }
        foreach my $platform (@platforms::LIST) {
            my $platformname = $platform->{name};
            my $arch = $platform->{arch};
            # Skip this if we're doing a local package for a different architecture.
            next if $is_local && $arch ne $local_arch;
            my $destdir = "$gebuilddir/$arch";
            my $rpmfile = "$destdir/$rpmname-$version.$arch.rpm";
            if (-f $rpmfile) {
                $RPMArchFiles{$rpmname,$platformname} = $rpmfile;
            } else {
                warn "Unable to find $rpmname-$version.$arch.rpm for $platformname\n";
                $SomeMissing = 1;
            }
        }
    }
}
if ($SomeMissing) {
    die "Unable to proceed\n";
}
if (! defined $distver) {
    die "INTERNAL ERROR: Unable to determine dist version\n";
}


print "\n========== Gather RPMs for packages ==========\n";
my @PackageInfo;
{
    my ($packname, $packageref);
    while (($packname, $packageref) = each %packages::LIST) {
        next if $only_packname ne '' && $only_packname ne $packname;

        my $platforms = $packageref->{platforms};
        my $prodlist = $packageref->{products } ;


        my %currprodhash;
        foreach my $prodname (@{$prodlist}) {
            if (! exists $Product::AllProductHash{$prodname}) {
                die "Unrecognized product ($prodname) listed for package $packname\n";
            }
            $currprodhash{$prodname} = 1;
        }
        my $thispack = {
            name   => $packname,
            prods  => { },
            rpms   => { },
            platforms => $platforms,
        };
        push @PackageInfo, $thispack;

        foreach my $product (@Product::AllProductList) {
            my $prodname = ref($product);
            if ($product->IsDeprecated()) {
                $thispack->{prods}{$prodname} = 1;
            } elsif (exists $currprodhash{$prodname}) {
                foreach my $excl ($product->ExclusiveList()) {
                    if (exists $thispack->{prods}{$excl}) {
                        die "PACKAGING ERROR: $prodname and $excl both in package $packname\n";
                    }
                }
                $thispack->{prods}{$prodname} = 1;
                foreach my $rpmname ($product->RPMList()) {
                    $thispack->{rpms}{$rpmname} = 1;
                }
            }
        }
    }
}

print "\n========== Making output directory ==========\n";
my $outdir = "$gedistdir/$distver";
if (-e $outdir) {
    if ($overwrite) {
        print "Pruning old output directory\n";
        rmtree($outdir,
               0,  # verbose
               0); # permission denied is OK
    } else {
        die "$outdir already exists.\nRefusing to proceed.\n";
    }
}
mkpath($outdir); # will croak if unable to create


print "\n========== Making package images ==========\n";
{
    foreach my $package (@PackageInfo) {
        my $packname = $package->{name};
        next if $only_packname ne '' && $only_packname ne $packname;
        # make top level dir
        my $packdir = "$outdir/$packname";
        mkpath($packdir);


        # make needed RPM dirs
        my $srpmdir = "$packdir/SRPMS";
        mkpath($srpmdir);
        my $rpmdir = "$packdir/RPMS";
        mkpath($rpmdir);
        mkpath("$rpmdir/noarch");
        foreach my $platformname (@{$package->{platforms}}) {
            # Skip this if we're doing a local package for a different architecture.
            next if $is_local && $platformname ne $local_arch;

            if (exists $platforms::HASH{$platformname}) {
                mkpath("$rpmdir/$platformname");
            }
        }

        # copy rpms
        foreach my $rpmname (keys %{$package->{rpms}}) {
            my $info;
            if (exists $rpminfo::NOARCH{$rpmname}) {
                $info = $rpminfo::NOARCH{$rpmname};
                my $rpmfile = $RPMNoArchFiles{$rpmname};
                lnorcp($rpmfile, "$rpmdir/noarch");
            } else {
                $info = $rpminfo::ARCH{$rpmname};
                if (!defined $info) {
                    die "Missing rpminfo.pm entry for $rpmname\n";
                }
                foreach my $platformname (@{$package->{platforms}}) {
                    # Skip this if we're doing a local package for a different architecture.
                    next if $is_local && $platformname ne $local_arch;
                    if (exists $platforms::HASH{$platformname}) {
                        my $rpmfile = $RPMArchFiles{$rpmname,$platformname};
                        lnorcp($rpmfile, "$rpmdir/$platformname");
                    }
                }
            }
            if ($info->{srpm}) {
                my $srpm = FindSRPM($rpmname, $info);
                if (! defined $srpm) {
                    die "Unable to find SRPM for $rpmname\n";
                }
                lnorcp($srpm, $srpmdir);
            }
        }

        # copy the installer files
        CopyInstallerFiles($packdir, $package);
    }
}


print "\n========== Fixing permissions ==========\n";
assertsystem("chmod -R a+r $outdir");
assertsystem("find $outdir -type d -print0 | " .
             "xargs -r --null chmod g+w");


print "\nDONE with $outdir\n";



sub lnorcp {
    my ($src, $dest) = @_;
    if (-d $dest) {
        $dest .= "/" . basename($src);
    }
    if (link($src, $dest)) {
        print "+ ln $src $dest\n" if $verbose;
    } elsif (File::Copy::copy($src, $dest)) {
        print "+ cp $src $dest\n" if $verbose;
    } else {
        die "Unable to copy $src to $dest: $!\n";
    }
}


sub assertsystem
{
    my $cmd = shift;
    if ($verbose) {
	print "+ $cmd\n";
    }
    system($cmd) == 0 || die "\n";
}

sub FindSRPM {
    my ($rpmname, $rpminfo) = @_;
    my $basedir = "$gebuilddir/../SRPMS";
    my $version = $rpminfo->{ver};
    my $srpm = "$basedir/$rpmname-$version.src.rpm";
    if (-f $srpm) {
        return $srpm;
    }
    while ($rpmname =~ s/-[^-]+$//) {
        my $srpm = "$basedir/$rpmname-$version.src.rpm";
        if (-f $srpm) {
            return $srpm;
        }
    }

    return undef;
}


sub CopyInstallerFiles {
    my ($packdir, $package) = @_;

    # copy the installer subversion directory
    # and prune files we know we don't want
    assertsystem("cp -a $FindBin::Bin/../installer/* $packdir");
    assertsystem("find $packdir -name .svn -print0 | " .
                 "xargs -r --null rm -rf");
    assertsystem("find $packdir -name \\*~ -print0 | " .
                 "xargs -r --null rm -rf");

    # now set the IsInstallable flags for each product
    foreach my $prodfile (grep(chomp $_,
                               `find $packdir -name \\*.product`)) {
        my $prodname = basename($prodfile, '.product');
        my $product = $Product::AllProductHash{$prodname};
        if (!$product->IsDeprecated() &&
            exists($package->{prods}{$prodname})) {
            my $primaryrpm = $product->PrimaryRPM();
            my $availver;
            if (exists $rpminfo::NOARCH{$primaryrpm}) {
                $availver = $rpminfo::NOARCH{$primaryrpm}->{ver};
            } else {
                $availver = $rpminfo::ARCH{$primaryrpm}->{ver};
            }
            open(PRODFILE, ">> $prodfile") ||
                die "Unable to open $prodfile for appending: $!\n";
            print PRODFILE "\n";
            print PRODFILE "sub AvailableInPackage { return 1; }\n";
            print PRODFILE "sub AvailableVersion { return '$availver'; }\n";
            print PRODFILE "1;\n";
            close(PRODFILE);
        }
    }

    # remove write permissions from all leaf files
    chmod(0555, "$packdir/InstallGEEnterprise.pl");
    assertsystem("find $packdir/lib -type f -print0 | " .
                 "xargs -r --null chmod a-w");
    assertsystem("find $packdir/products -type f -print0 | " .
                 "xargs -r --null chmod a-w");
}
