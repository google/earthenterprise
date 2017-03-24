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
use FindBin qw($Script);
use File::Path;
use File::Basename;

my $host;
my $project = 'vector/vector_layers';
my $user;
my $tmpdir = $ENV{TMPDIR};
$tmpdir = '/tmp' unless defined $tmpdir;
my $help = 0;
my $dryrun = 0;
my $fusionbin = '/opt/google/bin';


sub usage {
    warn "usage: $Script [options]\n";
    warn "   Push translations from perforce to fusion\n";
    warn "Valid options are:\n";
    warn "  --help               Display this message\n";
    warn "  --host <host>        Fusion machine w/ access to assetroot\n";
    warn "  --project <project>  Project to export (default: $project)\n";
    warn "  --user <user>        ssh user on Fusion machine (default: current user)\n";
    warn "  --tmpdir <tmpdir>    tmp dir for files (default: $tmpdir)\n";
    warn "  --dryrun             Don't do work. Just report\n";
        
    exit(1);
}


my $pwd = `pwd`;
chomp($pwd);

my $perforcedir = 'perforcelocation/googledata/localization/GoogleEarthLayers';
die "You must run $Script from $perforcedir\n"
    unless $pwd =~ /$perforcedir$/;


GetOptions('host=s'       => \$host,
           'project=s'    => \$project,
           'user=s'       => \$user,
           'tmpdir=s'     => \$tmpdir,
           'help'         => \$help,
           'dryrun'       => \$dryrun,
           ) || usage();
usage() if $help;

if (!defined $host) {
    warn "You must specify --host\n\n";
    usage();
}
my $userhost = defined($user) ? "$user\@$host" : "$host";



my $srcLayerFName;
my $srcLabelFName;
my $destLayerFName;
my $destLabelFName;
my $extraArgs = '';

my $remotetmpdir = "$tmpdir/PushTrans$$";

my $numChanged = 0;

SSHCommand("rm -rf $remotetmpdir");

# ********** Layers **********
# Gather list of localized LayerNames files
my @layerfiles = glob "LayerNames/locales/LayerNames_*.xlb";

# push LayerName files to fusion host
SSHCommand("mkdir -p $remotetmpdir");
SCPPut(\@layerfiles, $remotetmpdir);

# import the LayerNames into fusion
SSHCommand("$fusionbin/geimportvectorl10n --proj '$project' $remotetmpdir/\\*.xlb");
SSHCommand("rm -rf $remotetmpdir");


# ********** Labels **********
# Gather list of localized LabelNames files
my @labelfiles = glob "LabelNames/locales/LabelNames_*.xlb";

# push LabelName files to fusion host
SSHCommand("mkdir -p $remotetmpdir");
SCPPut(\@labelfiles, $remotetmpdir);

# import the LabelNames into fusion
SSHCommand("$fusionbin/geimportlabelsl10n $remotetmpdir/\\*.xlb");
SSHCommand("rm -rf $remotetmpdir");

sub SCPGet {
    my ($src, $dest, $opt) = @_;
    $opt = '' unless defined($opt);

    my $cmd = "scp $opt $userhost:$src $dest";
    print "+ $cmd\n";
    if (system($cmd) != 0) {
        die "Copy failed\n";
    }
}

sub SCPPut {
    my ($src, $dest, $opt) = @_;
    $opt = '' unless defined($opt);

    my $cmd = "scp $opt @{$src} $userhost:$dest";
    print "+ $cmd\n";
    if (system($cmd) != 0) {
        die "Copy failed\n";
    }
}

sub SSHCommand {
    my ($cmd) = @_;

    my $mycmd = "ssh $userhost $cmd";
    print "+ $mycmd\n";
    if (system($mycmd) != 0) {
        die "Remote command failed\n";
    }
}

sub Command {
    my ($cmd) = @_;

    print "+ $cmd\n";
    if (system($cmd) != 0) {
        die "Command failed\n";
    }
}
