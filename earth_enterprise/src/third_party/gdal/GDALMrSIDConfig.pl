#!/usr/bin/perl -w-

use strict;
use Getopt::Long;

our $usage = <<EOF;
usage: $0 [options] <var>
    Valid options are:
    --help                 Print this help message
    --version <versionstr> MrSID version string

    Valid options for <var> are:
    platform               MrSID platform specification
    withmrsid              GDAL configure --with_mrsid entry
EOF

my $help = 0;
my $version = '';
GetOptions("version=s" => \$version,
	   ) || die $usage;
die $usage if $help;

my $var = shift;
die $usage unless defined $var;

my $gccver = join('.', (split(/\./, `/opt/google/bin/gcc -dumpversion`))[0,1]);
my $machine = `uname -m`;

my $platform = 'unknown';
my $withmrsid = '--with-mrsid=no';
if ($gccver >= 4.1) {
  if ($machine =~ /x86_64/) {
    $platform = 'linux.x86-64.gcc41';
    $withmrsid = '--with-mrsid=`pwd`/Geo_DSDK-' . $version;
  } elsif (($machine =~ /i686/) || ($machine =~ /athlon/)) {
    $platform = 'linux.x86.gcc41';
    $withmrsid = '--with-mrsid=`pwd`/Geo_DSDK-' . $version;
  }
}

if ($var eq 'platform') {
    print "$platform\n";
} elsif ($var eq 'withmrsid') {
    print "$withmrsid\n";
} else {
    warn "Unrecognised variable request: $var\n";
    die $usage;
}

