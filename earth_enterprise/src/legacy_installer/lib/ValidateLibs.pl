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
my $altlibdir = '';

my %ignore_libs = (
    libjpeg => 1,  # jdk-ge supplies a version that nobody but it links to
);


sub usage {
    warn "usage: $Script [options]\n";
    warn "   Check for missing shared libraries.\n";
    warn "Valid options are:\n";
    warn "  --help               Display this message\n";
    warn "  --bindir <dir>       Check all executables under <dir>\n";
    warn "                       [default '$bindir']\n";
    warn "  --altlibdir <dir>    Alternate directory to check for libs\n";
    warn "                       [default '$altlibdir']\n";
        
    exit(1);
}

GetOptions('bindir=s'     => \$bindir,
           'altlibdir=s'  => \$altlibdir,
           'help'         => \$help,
           ) || usage();
usage() if $help;


$| = 1;
    

# find all supplied libs under /opt/google and altlibdir
my %libs_hash;
{
    if (length $altlibdir) {
        FindLibs($altlibdir, \%libs_hash);
    }
    FindLibs('/opt/google', \%libs_hash);
}


# find all missing libs
open(FIND, "find $bindir -type f -perm -u=x -print |");
# amin: for some reason on grhat we have to set the LD_LIBRARY_PATH to
# the altlibdir. other platforms somehow find the path correctly (maybe
# jdjohnso magic)
if (length $altlibdir) {
    $ENV{'LD_LIBRARY_PATH'} = $altlibdir;
}
while (<FIND>) {
    chomp;
    CheckExecutable($_, \%libs_hash);
}



# report all missing libs
my $bad = 0;
if (scalar %missing) {
    foreach my $key (keys %missing) {
        print STDERR "Missing $key:\n";
        my $count = @{$missing{$key}};
        my $report_num = $count;
        if ($report_num > 5) {
            $report_num = 5;
        }
        foreach my $exec (@{$missing{$key}}[0..$report_num-1]) {
            print STDERR "\t$exec\n";
        }
        if ($report_num < $count) {
            print STDERR "\tAnd ", $count-$report_num, " more ...\n";
        }
    }

    $bad = 1;
}
if (scalar %wrong) {
    foreach my $key (keys %wrong) {
        print STDERR "Wrong $key (should be $libs_hash{$key}):\n";
        my $count = @{$wrong{$key}};
        my $report_num = $count;
        if ($report_num > 5) {
            $report_num = 5;
        }
        my $maxlen = 0;
        foreach my $wrong (@{$wrong{$key}}[0..$report_num-1]) {
            my ($exec, $lib) = split(':', $wrong);
            my $len = length($exec);
            if ($len > $maxlen) {
                $maxlen = $len;
            }
        }
        foreach my $wrong (@{$wrong{$key}}[0..$report_num-1]) {
            my ($exec, $lib) = split(':', $wrong);
            print STDERR "\t$exec: ", ' 'x($maxlen-length($exec)), $lib, "\n";
        }
        if ($report_num < $count) {
            print STDERR "\tAnd ", $count-$report_num, " more ...\n";
        }
    }

    $bad = 1;
}
if (scalar %double) {
    foreach my $key (keys %double) {
        my ($exec, $baselib) = split(/$;/, $key);
        print STDERR "$exec has two copies of $baselib:\n";
        foreach my $ver (@{$double{$key}}) {
            print STDERR "\t$ver\n";
        }
    }

    $bad = 1;
}




exit $bad;



sub FindLibs {
    my ($basedir, $hash) = @_;;

    open(FIND, "find $basedir -name mod_\\*.so -prune -o \\( -name \\*.so -o -name \\*.so.\\* \\) -print |");
    while (<FIND>) {
        chomp;
        my $file = abs_path($_);

        my $base = basename $file;
        $base =~ s/\.so(\.[^\.]+)*$//;

        next if (exists $ignore_libs{$base});

        if (-l $file) {
            my $real = readlink($file);
            if ($real !~ /^\//) {
                $real = dirname($file) . "/$real";
            }
            $real = abs_path($real);
            $symlink_libs{$file} = $real;
            next;
        }

        # This will only pick the first of each one we see
        # this means altlibdir versions will win, which is what we want
        # It also means that libs with multipls versions (e.g. libawt)
        # will artificially pick the first one. That should be OK since
        # those with multiple versions almost certianly need their own way to
        # dynamically load the variant and will not have them linked into the
        # executable. Of cource I could be wrong, at which point I'll be
        # back to revisit this. :-)
        if (!exists($hash->{$base})) {
            $hash->{$base} = $file;
        }
    }
    close(FIND);
}


sub DumpLibs {
    my ($hash) = @_;
    foreach my $base (sort keys %{$hash}) {
        print "$base: $hash->{$base}\n";
    }
}



sub CheckExecutable {
    my ($exec, $libs_hash) = @_;
    my %found;

    # skip the libraries, we'll pick them up with any executables
    # that use them. If none use them, then we don't care
    if ($exec !~ /mod_.*\.so/) {
        return if $exec =~ /\.so$/;
        return if $exec =~ /\.so\./;
    }

    open(OUTPUT, "ldd $exec 2>&1 |");
    while (<OUTPUT>) {
        chomp;
        if (/^\s*(.*)\s+=>\s+not found/) {
            my $lib = $1;
            my ($base) = ($lib =~ /^(.+)\.so/);

            if ((length($altlibdir) > 0) &&
                exists($libs_hash->{$base}) &&
                ($libs_hash->{$base} =~ /^$altlibdir/)) {
                # even though it's missing now, it's OK
                # This run is at packaging time and the missing lib
                # was found in our pre-installed $altlibdir
            } else {
                push @{$missing{$base}}, $exec;
            }
        } elsif (/^\s*(.*)\s+=>\s+(.*\.so(\.[^\.\s]+)*)/) {
            my $lib = $1;
            my $result = $2;
            my ($base) = ($lib =~ /^(.+)\.so/);

            # bind symlinks to real files
            $result = abs_path($result);
            while (exists($symlink_libs{$result})) {
                $result = $symlink_libs{$result};
            }

            if (exists $found{$base}) {
                if ($found{$base} ne $result) {
                    $double{$exec,$base} = [ $found{$base}, $result ];
                }
            } else {
                $found{$base} = $result;
            }

            if (exists($libs_hash->{$base}) &&
                ($libs_hash->{$base} ne $result)) {
                # It's not what it should be
                # check for altlibdir
                if ((length($altlibdir) > 0) &&
                    exists($libs_hash->{$base}) &&
                    ($libs_hash->{$base} =~ /^$altlibdir/)) {
                    # Even though it's wrong now, it's OK
                    # This run is at packaging time and the incorrect lib
                    # was found in our pre-installed $altlibdir
                } else { 
                    push @{$wrong{$base}}, "$exec:$result";
                }
            }
        }
    }
    close(OUTPUT);
}
