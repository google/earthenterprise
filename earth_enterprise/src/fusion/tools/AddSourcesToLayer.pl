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


use strict;
use Getopt::Long;
use FindBin;
use lib $FindBin::Bin;
use KeyholeConfigFile;
use File::Basename;
use Cwd qw( realpath );

my $infile;
my $outfile;
my $layerid;
my $selectid;
my $filelist;
my $help = 0;

sub usage
{
    die "\nusage: $FindBin::Script --input <project file> --output <filename> --layerid <numeric layer id> --selectid <numerid select id> [--filelist <filename>] [sourcefile ...]\n\nWhere <select id> identifies the 'Select' that will serve as the prototype for the additions.\n\n";
}
GetOptions("input=s"    => \$infile,
           "output=s"   => \$outfile,
           "layerid=i"  => \$layerid,
           "selectid=i" => \$selectid,
           "filelist=s" => \$filelist,
           "help|?"     => \$help,
           ) || usage();
usage() if $help;
usage() unless defined($infile) && defined($outfile) && defined($layerid) &&
    defined($selectid);
die "Unable to read $filelist\n" if (defined $filelist && ! -r $filelist);


my $conf = new KeyholeConfigFile($infile);
foreach my $sourcefile (@ARGV) {
    my $path = Cwd::realpath($sourcefile);
    die "Cannot find file $path\n" unless -f $path;
    $conf->AddSourceToLayer($path, $layerid, $selectid)
}
if (defined $filelist) {
    my $dirname = dirname($filelist);
    my $cwd = Cwd::getcwd();
    chdir($dirname);
    open (FILELIST, basename($filelist)) || die "Uable to open filelist: $!\n";
    while (<FILELIST>) {
        chomp;
        my $path = Cwd::realpath($_);
        die "Cannot find file $path\n" unless -f $path;
        $conf->AddSourceToLayer($path, $layerid, $selectid)
    }
    close(FILELIST);
    chdir($cwd);
}
$conf->Save($outfile);


