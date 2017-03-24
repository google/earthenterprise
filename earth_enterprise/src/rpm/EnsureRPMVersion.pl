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
use lib $FindBin::Bin;
use lib "$FindBin::Bin/../installer/lib";
use RPMHelps;
use RPMVersion;

my $rpm = shift;
my $verstr = shift;

die "usage: EnsureRPMVersion.pl <rpmname> <minversion>\n"
    unless defined($rpm) && defined($verstr);

EnsureInstalledRPMVersion($rpm, $verstr);

sub GetPlatform
{
    if (open(LSB, '/etc/lsb-release')) {
        while (<LSB>) {
            return "ubuntu" if (/Ubuntu/);
        }
        close(LSB);
    }

    my $uname = `uname -m`;
    chomp $uname;
    return $uname;
}

sub EnsureInstalledRPMVersion
{
    my $platform = GetPlatform();
    if ($platform eq 'ubuntu') {
        return 0;
    }
    my ($rpm, $verrelstr) = @_;
    my $origverrelstr = $verrelstr;
    if ($verrelstr !~ /-/) {
        $verrelstr .= '-0';
    }
    my $instverstr = RPMHelps::InstalledVersion($rpm);
    if (!defined $instverstr) {
	die "$rpm is not installed\n  - Must have at least version $origverrelstr\n";
    }
    my $installedver = new RPMVersion($instverstr);
    my $minimumver   = new RPMVersion("$verrelstr");

    if (RPMVersion::Compare($installedver, $minimumver) < 0) {
	die "Installed $rpm ($installedver->{fullver}) is too old\n  - Must have at least version $origverrelstr\n";
    }
}
