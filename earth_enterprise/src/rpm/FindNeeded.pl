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
use File::Basename;
use Cwd 'abs_path';
use FindBin qw($Script);

my %missing;
my %wrong;
my %double;
my %symlink_libs;
my $help = 0;
my $bindir    = '/opt/google';
my $showrpms = 0;
my %rpms;


sub usage {
    warn "usage: $Script [options] <pattern>\n";
    warn "   Check for NEEDED entries in executables and shared libraries.\n";
    warn "Valid options are:\n";
    warn "  --help               Display this message\n";
    warn "  --bindir <dir>       Check all executables under <dir>\n";
    warn "                       [default '$bindir']\n";
        
    exit(1);
}

GetOptions('bindir=s'     => \$bindir,
           'help'         => \$help,
           'rpms'         => \$showrpms,
           ) || usage();
usage() if $help;
my $pattern = shift;
usage()  unless defined $pattern;


$| = 1;
    


# find all missing libs
open(FIND, "find $bindir -type f -perm -u=x -print |");
while (<FIND>) {
    chomp;
    CheckExecutable($_, $pattern);
}



sub CheckExecutable {
    my ($exec, $pattern) = @_;

    next if -l $exec;

    open(OUTPUT, "objdump -p $exec 2>&1 |");
    while (<OUTPUT>) {
        chomp;
        if (/\s*NEEDED\s+(.*)/) {
            my $needed = $1;
            if ($needed =~ /$pattern/) {
                if ($showrpms) {
                    ReportRPM($exec);
                } else {
                    print $exec, "\n";
                }
            }
        }
    }
    close(OUTPUT);
}

sub ReportRPM {
    my ($exec) = @_;

    open(RPM, "rpm -qf $exec 2>/dev/null |");
    my $rpm = <RPM>;
    close(RPM);
    if (defined $rpm) {
        chomp $rpm;
        if (!exists $rpms{$rpm}) {
            print $rpm, "\n";
            $rpms{$rpm} = 1;
        }
    } else {
        print $exec, "\n";
    }
}
